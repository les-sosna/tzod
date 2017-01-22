#include "CampaignControls.h"
#include "Controller.h"
#include "Game.h"
#include "InputManager.h"
#include "MessageArea.h"
#include "ScoreTable.h"
#include "inc/shell/Config.h"

#include <ctx/Deathmatch.h>
#include <ctx/GameContext.h>
#include <ctx/WorldController.h>
#include <gc/Player.h>
#include <gc/Vehicle.h>
#include <gc/World.h>
#include <loc/Language.h>
#include <ui/DataSource.h>
#include <ui/InputContext.h>
#include <ui/GuiManager.h>
#include <ui/Keys.h>
#include <ui/LayoutContext.h>
#include <ui/StackLayout.h>
#include <ui/UIInput.h>
#include <video/DrawingContext.h>
#include <video/TextureManager.h>

#include <sstream>

namespace
{
	class TimerDisplay : public UI::DataSource<const std::string&>
	{
	public:
		TimerDisplay(World &world)
			: _world(world)
		{}

		// UI::DataSource<const std::string&>
		const std::string& GetValue(const UI::StateContext &sc) const override
		{
			std::ostringstream text;
			int time = (int)_world.GetTime();
			text << (time / 60) << ":";
			if (time % 60 < 10)
				text << "0";
			text << (time % 60);

			_cachedString = text.str();

			return _cachedString;
		}

	private:
		World &_world;
		mutable std::string _cachedString;
	};
}

///////////////////////////////////////////////////////////////////////////////

GameLayout::GameLayout(UI::LayoutManager &manager,
                       TextureManager &texman,
                       GameContext &gameContext,
                       WorldView &worldView,
                       WorldController &worldController,
                       ConfCache &conf,
                       LangCache &lang,
                       UI::ConsoleBuffer &logger)
  : Window(manager)
  , _gameContext(gameContext)
  , _gameViewHarness(gameContext.GetWorld(), worldController)
  , _worldView(worldView)
  , _worldController(worldController)
  , _conf(conf)
  , _lang(lang)
  , _inputMgr(conf, logger)
  , _texDrag(texman.FindSprite("ui/direction"))
  , _texTarget(texman.FindSprite("ui/target"))
  , _scoreAndControls(std::make_shared<UI::StackLayout>(manager))
{
	_scoreAndControls->SetSpacing(20);
	_scoreAndControls->SetAlign(UI::Align::CT);
	AddFront(_scoreAndControls);

	_msg = std::make_shared<MessageArea>(manager, texman, _conf, logger);
	AddFront(_msg);

	auto deathmatch = dynamic_cast<Deathmatch*>(_gameContext.GetGameplay());
	_score = std::make_shared<ScoreTable>(manager, texman, _gameContext.GetWorld(), deathmatch, _lang);
	_score->SetVisible(false);
	_scoreAndControls->AddFront(_score);

	if (deathmatch)
	{
		_campaignControls = std::make_shared<CampaignControls>(manager, texman, *deathmatch);
		_campaignControls->SetVisible(false);
		_scoreAndControls->AddFront(_campaignControls);
	}

	_timerDisplay = std::make_shared<UI::Text>(manager, texman);
	_timerDisplay->SetAlign(alignTextRB);
	_timerDisplay->SetText(std::make_shared<TimerDisplay>(_gameContext.GetWorld()));
	AddFront(_timerDisplay);

	_conf.ui_showtime.eventChange = std::bind(&GameLayout::OnChangeShowTime, this);
	OnChangeShowTime();

	SetTimeStep(true);
	_gameContext.GetGameEventSource().AddListener(*this);
}

GameLayout::~GameLayout()
{
	_gameContext.GetGameEventSource().RemoveListener(*this);
	_conf.ui_showtime.eventChange = nullptr;
}

vec2d GameLayout::GetDragDirection() const
{
	vec2d dragDirection{ 0, 0 };

	if (!_activeDrags.empty())
	{
		float maxDragLength = 0;
		for (auto &drag : _activeDrags)
		{
			vec2d dir = drag.second.second - drag.second.first;
			maxDragLength = std::max(maxDragLength, dir.sqr());
		}
		maxDragLength = std::sqrt(maxDragLength);

		// only account for len > max / 2
		unsigned int count = 0;
		for (auto &drag : _activeDrags)
		{
			vec2d dir = drag.second.second - drag.second.first;
			if (dir.len() > maxDragLength/2)
			{
				dragDirection += dir;
				count++;
			}
		}
		dragDirection /= (float)count;
	}
	return dragDirection;
}

unsigned int GameLayout::GetEffectiveDragCount() const
{
	float maxDragLength = 0;
	for (auto &drag : _activeDrags)
	{
		vec2d dir = drag.second.second - drag.second.first;
		maxDragLength = std::max(maxDragLength, dir.sqr());
	}
	maxDragLength = std::sqrt(maxDragLength);
	
	// only account for len > max / 2
	unsigned int count = 0;
	for (auto &drag : _activeDrags)
	{
		vec2d dir = drag.second.second - drag.second.first;
		if (dir.len() > maxDragLength/2)
		{
			count++;
		}
	}
	return count;
}

void GameLayout::OnTimeStep(UI::LayoutManager &manager, float dt)
{
	bool tab = manager.GetInputContext().GetInput().IsKeyPressed(UI::Key::Tab);
	bool gameOver = _gameContext.GetGameplay() ? _gameContext.GetGameplay()->IsGameOver() : false;
	bool allDead = !_gameContext.GetWorldController().GetLocalPlayers().empty();
	for (auto player : _gameContext.GetWorldController().GetLocalPlayers())
		allDead &= !player->GetVehicle();
	_score->SetVisible(tab || gameOver || (allDead && _gameContext.GetWorld().GetTime() > PLAYER_RESPAWN_DELAY));
	if (_campaignControls)
		_campaignControls->SetVisible(gameOver);

	_gameViewHarness.Step(dt);

	bool readUserInput = false;
	for (auto wnd = manager.GetDesktop(); wnd; wnd = wnd->GetFocus())
	{
		if (this == wnd.get())
		{
			readUserInput = true;
		}
	}

	std::vector<GC_Player*> players = _worldController.GetLocalPlayers();
	
	for (auto player : players)
		player->SetIsActive(readUserInput);

	if (readUserInput)
	{
		WorldController::ControllerStateMap controlStates;

		vec2d dragDirection = GetDragDirection();
		bool reversing = GetEffectiveDragCount() > 1;
		
		for (unsigned int playerIndex = 0; playerIndex != players.size(); ++playerIndex)
		{
			if( Controller *controller = _inputMgr.GetController(playerIndex) )
			{
				controller->Step(dt);
				if( GC_Vehicle *vehicle = players[playerIndex]->GetVehicle() )
				{
					vec2d mouse = manager.GetInputContext().GetInput().GetMousePos();
					auto c2w = _gameViewHarness.CanvasToWorld(playerIndex, (int) mouse.x, (int) mouse.y);

					VehicleState vs;
					controller->ReadControllerState(manager.GetInputContext().GetInput(), _gameContext.GetWorld(),
					                                *vehicle, c2w.visible ? &c2w.worldPos : nullptr, dragDirection, reversing, vs);
					controlStates.insert(std::make_pair(vehicle->GetId(), vs));
				}
			}
		}

		_worldController.SendControllerStates(std::move(controlStates));
	}
}

void GameLayout::Draw(const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	const_cast<GameViewHarness&>(_gameViewHarness).SetCanvasSize((int)lc.GetPixelSize().x, (int)lc.GetPixelSize().y, lc.GetScale());

	_gameViewHarness.RenderGame(dc, _worldView);

	vec2d dir = GetDragDirection();
	bool reversing = GetEffectiveDragCount() > 1;
	std::vector<GC_Player*> players = _worldController.GetLocalPlayers();
	for (unsigned int playerIndex = 0; playerIndex != players.size(); ++playerIndex)
	{
		if (!dir.IsZero())
		{
			if (const GC_Vehicle *vehicle = players[playerIndex]->GetVehicle())
			{
				vec2d pos = _gameViewHarness.WorldToCanvas(playerIndex, vehicle->GetPos());
				pos += dir;
				uint32_t opacity = uint32_t(std::min(dir.len() / 200.f, 1.f) * 255.f) & 0xff;
				uint32_t rgb = reversing ? opacity : opacity << 8;
				dc.DrawSprite(_texDrag, 0, rgb | (opacity << 24), pos.x, pos.y, dir.Norm());
			}
		}
		
		if (const Controller *controller = _inputMgr.GetController(playerIndex))
		{
			float time = controller->GetRemainingFireTime();
			if (time > 0)
			{
				vec2d pos = _gameViewHarness.WorldToCanvas(playerIndex, controller->GetFireTarget());
				dc.DrawSprite(_texTarget, 0, 0xff00ff00, pos.x, pos.y, Vec2dDirection(_gameContext.GetWorld().GetTime()*3));
			}
		}
	}
}

FRECT GameLayout::GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::StateContext &sc, const UI::Window &child) const
{
	float scale = lc.GetScale();
	vec2d size = lc.GetPixelSize();

	if (_scoreAndControls.get() == &child)
	{
		vec2d pxChildSize = child.GetContentSize(texman, sc, lc.GetScale());
		return MakeRectWH(Vec2dFloor((size - pxChildSize) / 2), pxChildSize);
	}
	if (_timerDisplay.get() == &child)
	{
		return UI::CanvasLayout(size / scale, _timerDisplay->GetSize(), scale);
	}
	if (_msg.get() == &child)
	{
		return UI::CanvasLayout(vec2d{ 50, size.y / scale - 50 }, _msg->GetSize(), scale);
	}
	return UI::Window::GetChildRect(texman, lc, sc, child);
}

bool GameLayout::OnPointerDown(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, UI::PointerType pointerType, unsigned int pointerID)
{
	if (UI::PointerType::Touch == pointerType)
	{
		_activeDrags[pointerID].first = pointerPosition;
		_activeDrags[pointerID].second = pointerPosition;
		return true;
	}
	return false;
}

void GameLayout::OnPointerUp(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, UI::PointerType pointerType, unsigned int pointerID)
{
	_activeDrags.erase(pointerID);
}

void GameLayout::OnPointerMove(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, UI::PointerType pointerType, unsigned int pointerID, bool captured)
{
	if( captured )
	{
		auto &drag = _activeDrags[pointerID];
		drag.second = pointerPosition;
		vec2d dir = drag.second - drag.first;
		const float maxDragLength = 100;
		if (dir.len() > maxDragLength)
		{
			drag.first = drag.second - dir.Norm() * maxDragLength;
		}
	}
}

void GameLayout::OnTap(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition)
{
	std::vector<GC_Player*> players = _worldController.GetLocalPlayers();
	for (unsigned int playerIndex = 0; playerIndex != players.size(); ++playerIndex)
	{
		if( Controller *controller = _inputMgr.GetController(playerIndex) )
		{
			auto c2w = _gameViewHarness.CanvasToWorld(playerIndex, (int)pointerPosition.x, (int)pointerPosition.y);
			if (c2w.visible)
			{
				controller->OnTap(c2w.worldPos);
			}
		}
	}
}

void GameLayout::OnChangeShowTime()
{
	_timerDisplay->SetVisible(_conf.ui_showtime.Get());
}

void GameLayout::OnMurder(GC_Player &victim, GC_Player *killer, MurderType murderType)
{
	char msg[256] = { 0 };
	switch (murderType)
	{
	default:
		assert(false);
	case MurderType::Accident:
		snprintf(msg, sizeof(msg), _lang.msg_player_x_died.Get().c_str(), victim.GetNick().c_str());
		break;
	case MurderType::Enemy:
		assert(killer);
		snprintf(msg, sizeof(msg), _lang.msg_player_x_killed_his_enemy_x.Get().c_str(), killer->GetNick().c_str(), victim.GetNick().c_str());
		break;
	case MurderType::Friend:
		assert(killer);
		snprintf(msg, sizeof(msg), _lang.msg_player_x_killed_his_friend_x.Get().c_str(), killer->GetNick().c_str(), victim.GetNick().c_str());
		break;
	case MurderType::Suicide:
		snprintf(msg, sizeof(msg), _lang.msg_player_x_killed_him_self.Get().c_str(), victim.GetNick().c_str());
		break;
	}
	_msg->WriteLine(msg);
}
