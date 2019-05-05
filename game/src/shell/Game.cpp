#include "CampaignControls.h"
#include "Game.h"
#include "InputManager.h"
#include "MessageArea.h"
#include "ScoreTable.h"
#include "VehicleStateReader.h"
#include "inc/shell/Config.h"

#include <ctx/Deathmatch.h>
#include <ctx/GameContext.h>
#include <ctx/WorldController.h>
#include <gc/Player.h>
#include <gc/Vehicle.h>
#include <gc/World.h>
#include <loc/Language.h>
#include <plat/Input.h>
#include <plat/Keys.h>
#include <ui/DataSource.h>
#include <ui/InputContext.h>
#include <ui/GuiManager.h>
#include <ui/LayoutContext.h>
#include <ui/Rating.h>
#include <ui/Rectangle.h>
#include <ui/StackLayout.h>
#include <video/RenderContext.h>
#include <video/TextureManager.h>

#include <cfloat>
#include <iomanip>
#include <sstream>

namespace
{
	class TimerDisplay final
		: public UI::LayoutData<std::string_view>
	{
	public:
		TimerDisplay(World &world, const Deathmatch *deathmatch)
			: _world(world)
			, _deathmatch(deathmatch)
		{}

		// UI::LayoutData<std::string_view>
		std::string_view GetLayoutValue(const UI::DataContext &dc) const override
		{
			std::ostringstream text;
			if (_deathmatch && _deathmatch->GetTimeLimit() > 0)
			{
				int secondsLeft = (int)std::ceil(_deathmatch->GetTimeLimit() - _world.GetTime());
				text << (secondsLeft / 60) << ":" << std::setfill('0') << std::setw(2) << (secondsLeft % 60);
			}
			else
			{
				int seconds = (int)_world.GetTime();
				text << (seconds / 60) << ":";
				if (seconds % 60 < 10)
					text << "0";
				text << (seconds % 60);
			}

			_cachedString = text.str();

			return _cachedString;
		}

	private:
		World &_world;
		const Deathmatch *_deathmatch;
		mutable std::string _cachedString;
	};

	class DeathmatchCampaignRatingBinding final
		: public UI::RenderData<unsigned int>
	{
	public:
		DeathmatchCampaignRatingBinding(const GameContextCampaignDM &campaignContext)
			: _campaignContext(campaignContext)
		{}

		// UI::RenderData<unsigned int>
		unsigned int GetRenderValue(const UI::DataContext &dc, const UI::StateContext &sc) const override
		{
			return _campaignContext.GetRating();
		}

	private:
		const GameContextCampaignDM &_campaignContext;
	};
}

///////////////////////////////////////////////////////////////////////////////

constexpr float showScoreDelay = 0.3f;


GameLayout::GameLayout(UI::TimeStepManager &manager,
                       std::shared_ptr<GameContext> gameContext,
                       WorldView &worldView,
                       WorldController &worldController,
                       ShellConfig &conf,
                       LangCache &lang,
                       Plat::ConsoleBuffer &logger,
                       CampaignControlCommands campaignControlCommands)
  : UI::TimeStepping(manager)
  , _scoreAndControls(std::make_shared<UI::StackLayout>())
  , _gameContext(std::move(gameContext))
  , _gameViewHarness(_gameContext->GetWorld(), worldController)
  , _worldView(worldView)
  , _worldController(worldController)
  , _conf(conf)
  , _lang(lang)
  , _inputMgr(conf, logger)
{
	assert(_gameContext);

	_background = std::make_shared<UI::Rectangle>();
	_background->SetTexture("ui/list");
	AddFront(_background);

	_scoreAndControls->SetSpacing(20);
	_scoreAndControls->SetAlign(UI::Align::CT);
	AddFront(_scoreAndControls);

	_msg = std::make_shared<MessageArea>(manager, logger);
	AddFront(_msg);

	if ( auto campaignContext = dynamic_cast<GameContextCampaignDM*>(_gameContext.get()))
	{
		_rating = std::make_shared<UI::Rating>();
		_rating->SetRating(std::make_shared<DeathmatchCampaignRatingBinding>(*campaignContext));
		_rating->SetVisible(false);
		_scoreAndControls->AddFront(_rating);
	}

	auto deathmatch = dynamic_cast<Deathmatch*>(_gameContext->GetGameplay());
	_score = std::make_shared<ScoreTable>(_gameContext->GetWorld(), deathmatch, _lang);
	_score->SetVisible(false);
	_scoreAndControls->AddFront(_score);

	if (deathmatch)
	{
		_campaignControls = std::make_shared<CampaignControls>(*deathmatch, std::move(campaignControlCommands));
		_campaignControls->SetVisible(false);
		_scoreAndControls->AddFront(_campaignControls);
		_scoreAndControls->SetFocus(_campaignControls);
	}

	_timerDisplay = std::make_shared<UI::Text>();
	_timerDisplay->SetAlign(alignTextRB);
	_timerDisplay->SetText(std::make_shared<TimerDisplay>(_gameContext->GetWorld(), deathmatch));
	AddFront(_timerDisplay);

	SetTimeStep(true);
	_gameContext->GetGameEventSource().AddListener(*this);
}

GameLayout::~GameLayout()
{
	_gameContext->GetGameEventSource().RemoveListener(*this);
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

bool GameLayout::GetAllPlayerDead() const
{
	bool allDead = !_gameContext->GetWorldController().GetLocalPlayers().empty();
	for (auto player : _gameContext->GetWorldController().GetLocalPlayers())
		allDead &= (player->GetNumDeaths() > 0 && !player->GetVehicle());
	return allDead;
}

float GameLayout::GetLastPlayerDieTime() const
{
	float lastDieTime = FLT_MAX;
	for (auto player : _gameContext->GetWorldController().GetLocalPlayers())
	{
		if (player->GetNumDeaths() > 0)
		{
			if (lastDieTime == FLT_MAX)
				lastDieTime = 0;
			lastDieTime = std::max(lastDieTime, player->GetDieTime());
		}
	}
	return lastDieTime;
}

void GameLayout::OnTimeStep(const UI::InputContext &ic, float dt)
{
	float gameplayTime = _gameContext->GetGameplayTime();
	float gameOverTime = _gameContext->GetGameplay() ? _gameContext->GetGameplay()->GetGameOverTime() : FLT_MAX;
	float lastDieTime = GetLastPlayerDieTime();

	constexpr float showRatingDelay = 1.0f;
	constexpr float showControlsDelay = 2.0f;

	bool showScoreWhenDead = lastDieTime <= gameplayTime - showScoreDelay && GetAllPlayerDead();
	bool showScoreWhenGameOver = gameOverTime <= gameplayTime - showScoreDelay;
	bool showScoreWhenTabPressed = ic.GetInput().IsKeyPressed(Plat::Key::Tab);

	_score->SetVisible(showScoreWhenTabPressed || showScoreWhenDead || showScoreWhenGameOver);
	if (_campaignControls)
		_campaignControls->SetVisible(gameOverTime <= gameplayTime - showControlsDelay);
	if (_rating)
		_rating->SetVisible(gameOverTime <= gameplayTime - showRatingDelay);

	_gameViewHarness.Step(dt);

	std::vector<GC_Player*> players = _worldController.GetLocalPlayers();
	for (auto player : players)
		player->SetIsActive(ic.GetFocused());

	if (ic.GetFocused())
	{
		WorldController::ControllerStateMap controlStates;

		vec2d dragDirection = GetDragDirection();
		bool reversing = GetEffectiveDragCount() > 1;
		
		for (unsigned int playerIndex = 0; playerIndex != players.size(); ++playerIndex)
		{
			if(VehicleStateReader *vehicleStateReader = _inputMgr.GetVehicleStateReader(playerIndex) )
			{
				vehicleStateReader->Step(dt);
				if( GC_Vehicle *vehicle = players[playerIndex]->GetVehicle() )
				{
					VehicleState vs;
					vehicleStateReader->ReadVehicleState(_gameViewHarness, *vehicle, playerIndex,
						ic.GetInput(), dragDirection, reversing, vs);
					controlStates.insert(std::make_pair(vehicle->GetId(), vs));
				}
			}
		}

		_worldController.SendControllerStates(std::move(controlStates));
	}
}

void GameLayout::Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const
{
	int pxWidth = (int)lc.GetPixelSize().x;
	int pxHeight = (int)lc.GetPixelSize().y;
	float scale = std::min(lc.GetPixelSize().x / 1024.f, lc.GetPixelSize().y / 768.f); // lc.GetScale()
	const_cast<GameViewHarness&>(_gameViewHarness).SetCanvasSize(pxWidth, pxHeight, scale);

	_gameViewHarness.RenderGame(rc, _worldView, _conf.d_field.Get(), _conf.d_path.Get() ? &_gameContext->GetAIManager() : nullptr);

	// On-screen controls
	vec2d dir = GetDragDirection();
	bool reversing = GetEffectiveDragCount() > 1;
	std::vector<GC_Player*> players = _worldController.GetLocalPlayers();
	for (unsigned int playerIndex = 0; playerIndex != players.size(); ++playerIndex)
	{
		// Draw touch drag indicator
		if (!dir.IsZero())
		{
			if (const GC_Vehicle *vehicle = players[playerIndex]->GetVehicle())
			{
				vec2d pos = _gameViewHarness.WorldToCanvas(playerIndex, vehicle->GetPos());
				pos += dir;
				uint32_t opacity = uint32_t(std::min(dir.len() / 200.f, 1.f) * 255.f) & 0xff;
				uint32_t rgb = reversing ? opacity : opacity << 8;
				rc.DrawSprite(_texDrag.GetTextureId(texman), 0, rgb | (opacity << 24), pos, dir.Norm());
			}
		}
		
		// Draw tap target
		if (const VehicleStateReader *vehicleStateReader = _inputMgr.GetVehicleStateReader(playerIndex))
		{
			float time = vehicleStateReader->GetRemainingFireTime();
			if (time > 0)
			{
				vec2d pos = _gameViewHarness.WorldToCanvas(playerIndex, vehicleStateReader->GetFireTarget());
				rc.DrawSprite(_texTarget.GetTextureId(texman), 0, 0xff00ff00, pos, Vec2dDirection(_gameContext->GetWorld().GetTime()*3));
			}
		}
	}
}

FRECT GameLayout::GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const
{
	float scale = lc.GetScale();
	vec2d size = lc.GetPixelSize();

	if (_background.get() == &child)
	{
		if (_gameContext->GetGameplay() && _gameContext->GetGameplay()->GetGameOverTime() <= _gameContext->GetGameplayTime())
			return MakeRectWH(size);
		else
			return GetChildRect(texman, lc, dc, *_scoreAndControls);
	}
	if (_scoreAndControls.get() == &child)
	{
		vec2d pxChildSize = child.GetContentSize(texman, dc, lc.GetScale(), DefaultLayoutConstraints(lc));
		return MakeRectWH(Vec2dFloor((size - pxChildSize) / 2), pxChildSize);
	}
	if (_timerDisplay.get() == &child)
	{
		return MakeRectWH(size - vec2d{1, 1}, {});
	}
	if (_msg.get() == &child)
	{
		return UI::CanvasLayout(vec2d{ 50, size.y / scale - 50 }, _msg->GetSize(), scale);
	}
	return UI::Window::GetChildRect(texman, lc, dc, child);
}

float GameLayout::GetChildOpacity(const UI::Window& child) const
{
	if (_background.get() == &child)
	{
		float gameplayTime = _gameContext->GetGameplayTime();
		float gameOverTime = _gameContext->GetGameplay() ? _gameContext->GetGameplay()->GetGameOverTime() : FLT_MAX;
		return _score->GetVisible() ? 1 : std::clamp((gameplayTime - gameOverTime) / showScoreDelay, 0.f, 1.f);
	}
	return 1;
}

std::shared_ptr<UI::Window> GameLayout::GetFocus() const
{
	return _scoreAndControls;
}

bool GameLayout::OnPointerDown(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, int button)
{
	if (Plat::PointerType::Touch == pi.type)
	{
		_activeDrags[pi.id].first = pi.position;
		_activeDrags[pi.id].second = pi.position;
		return true;
	}
	return false;
}

void GameLayout::OnPointerUp(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, int button)
{
	_activeDrags.erase(pi.id);
}

void GameLayout::OnPointerMove(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, bool captured)
{
	if( captured )
	{
		auto &drag = _activeDrags[pi.id];
		drag.second = pi.position;
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
		if( VehicleStateReader *vehicleStateReader = _inputMgr.GetVehicleStateReader(playerIndex) )
		{
			auto c2w = _gameViewHarness.CanvasToWorld(playerIndex, (int)pointerPosition.x, (int)pointerPosition.y);
			if (c2w.visible)
			{
				vehicleStateReader->OnTap(c2w.worldPos);
			}
		}
	}
}

void GameLayout::OnMurder(GC_Player &victim, GC_Player *killer, MurderType murderType)
{
	char msg[256] = { 0 };
	switch (murderType)
	{
	default:
		assert(false);
	case MurderType::Accident:
		// TODO: remove string allocations
		snprintf(msg, sizeof(msg), std::string(_lang.msg_player_x_died.Get()).c_str(), std::string(victim.GetNick()).c_str());
		break;
	case MurderType::Enemy:
		assert(killer);
		// TODO: remove string allocations
		snprintf(msg, sizeof(msg), std::string(_lang.msg_player_x_killed_his_enemy_x.Get()).c_str(), std::string(killer->GetNick()).c_str(), std::string(victim.GetNick()).c_str());
		break;
	case MurderType::Friend:
		assert(killer);
		// TODO: remove string allocations
		snprintf(msg, sizeof(msg), std::string(_lang.msg_player_x_killed_his_friend_x.Get()).c_str(), std::string(killer->GetNick()).c_str(), std::string(victim.GetNick()).c_str());
		break;
	case MurderType::Suicide:
		// TODO: remove string allocations
		snprintf(msg, sizeof(msg), std::string(_lang.msg_player_x_killed_him_self.Get()).c_str(), std::string(victim.GetNick()).c_str());
		break;
	}
	_msg->WriteLine(msg);
}
