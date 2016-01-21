#include "Controller.h"
#include "DefaultCamera.h"
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
#include <ui/GuiManager.h>
#include <ui/Keys.h>
#include <ui/UIInput.h>
#include <video/DrawingContext.h>
#include <video/TextureManager.h>

#include <sstream>

TimeElapsed::TimeElapsed(UI::Window *parent, float x, float y, enumAlignText align, World &world)
  : Text(parent)
  , _world(world)
{
	SetTimeStep(true);
	Move(x, y);
	SetAlign(align);
}

void TimeElapsed::OnVisibleChange(bool visible, bool inherited)
{
	SetTimeStep(visible);
}

void TimeElapsed::OnTimeStep(float dt)
{
	std::ostringstream text;
	int time = (int) _world.GetTime();
	text << (time / 60) << ":";
	if( time % 60 < 10 )
		text << "0";
	text << (time % 60);
	SetText(text.str());
}

///////////////////////////////////////////////////////////////////////////////

GameLayout::GameLayout(Window *parent,
                       GameContext &gameContext,
                       WorldView &worldView,
                       WorldController &worldController,
                       const DefaultCamera &defaultCamera,
                       ConfCache &conf,
                       LangCache &lang,
                       UI::ConsoleBuffer &logger)
  : Window(parent)
  , _gameContext(gameContext)
  , _gameViewHarness(gameContext.GetWorld(), worldController)
  , _worldView(worldView)
  , _worldController(worldController)
  , _defaultCamera(defaultCamera)
  , _conf(conf)
  , _lang(lang)
  , _inputMgr(conf, logger)
  , _texDrag(GetManager().GetTextureManager().FindSprite("ui/direction"))
  , _texTarget(GetManager().GetTextureManager().FindSprite("ui/target"))
{
	_msg = new MessageArea(this, _conf, logger);
	_msg->Move(100, 100);

	_score = new ScoreTable(this, _gameContext.GetWorld(), _gameContext.GetGameplay(), _lang);
	_score->SetVisible(false);

	_time = new TimeElapsed(this, 0, 0, alignTextRB, _gameContext.GetWorld());
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
	vec2d dragDirection(0, 0);

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

void GameLayout::OnTimeStep(float dt)
{
	bool tab = GetManager().GetInput().IsKeyPressed(UI::Key::Tab);
	_score->SetVisible(tab || _gameContext.GetGameplay().IsGameOver());

	_gameViewHarness.Step(dt);

	bool readUserInput = !GetManager().GetFocusWnd() || this == GetManager().GetFocusWnd();
	WorldController::ControllerStateMap controlStates;

	if (readUserInput)
	{
		vec2d dragDirection = GetDragDirection();
		bool reversing = GetEffectiveDragCount() > 1;
		
		std::vector<GC_Player*> players = _worldController.GetLocalPlayers();
		for (unsigned int playerIndex = 0; playerIndex != players.size(); ++playerIndex)
		{
			if( Controller *controller = _inputMgr.GetController(playerIndex) )
			{
				controller->Step(dt);
				if( GC_Vehicle *vehicle = players[playerIndex]->GetVehicle() )
				{
					vec2d mouse = GetManager().GetInput().GetMousePos();
					auto c2w = _gameViewHarness.CanvasToWorld(playerIndex, (int) mouse.x, (int) mouse.y);

					VehicleState vs;
					controller->ReadControllerState(GetManager().GetInput(), _gameContext.GetWorld(),
					                                *vehicle, c2w.visible ? &c2w.worldPos : nullptr, dragDirection, reversing, vs);
					controlStates.insert(std::make_pair(vehicle->GetId(), vs));
				}
			}
		}

		_worldController.SendControllerStates(std::move(controlStates));
	}
}

void GameLayout::Draw(DrawingContext &dc) const
{
	Window::Draw(dc);

	vec2d eye(_defaultCamera.GetPos().x + GetWidth() / 2, _defaultCamera.GetPos().y + GetHeight() / 2);
	float zoom = _defaultCamera.GetZoom();
	_gameViewHarness.RenderGame(dc, _worldView, eye, zoom);
	dc.SetMode(RM_INTERFACE);

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
				dc.DrawSprite(_texTarget, 0, 0xff00ff00, pos.x, pos.y, vec2d(_gameContext.GetWorld().GetTime()*3));
			}
		}
	}
}

void GameLayout::OnSize(float width, float height)
{
	_score->Move(std::floor((width - _score->GetWidth()) / 2), std::floor((height - _score->GetHeight()) / 2));
	_time->Move(GetWidth() - 1, GetHeight() - 1);
	_msg->Move(_msg->GetX(), GetHeight() - 50);
	auto size = GetWidth() > GetHeight() ? GetWidth() : GetHeight();
	float base = 1024.f;
	float scale = size > base ? std::floor(size / base + 0.5f) : 1 / std::floor(base / size + 0.5f);
	_gameViewHarness.SetCanvasSize((int) GetWidth(), (int) GetHeight(), scale);
}

bool GameLayout::OnPointerDown(float x, float y, int button, UI::PointerType pointerType, unsigned int pointerID)
{
	if (UI::PointerType::Touch == pointerType)
	{
		_activeDrags[pointerID].first = vec2d(x, y);
		_activeDrags[pointerID].second = vec2d(x, y);
		GetManager().SetCapture(pointerID, this);
	}
	return true;
}

bool GameLayout::OnPointerUp(float x, float y, int button, UI::PointerType pointerType, unsigned int pointerID)
{
	if (GetManager().GetCapture(pointerID) == this)
	{
		_activeDrags.erase(pointerID);
		GetManager().SetCapture(pointerID, nullptr);
	}
	return true;
}

bool GameLayout::OnPointerMove(float x, float y, UI::PointerType pointerType, unsigned int pointerID)
{
	if (GetManager().GetCapture(pointerID) == this)
	{
		auto &drag = _activeDrags[pointerID];
		drag.second = vec2d(x, y);
		vec2d dir = drag.second - drag.first;
		const float maxDragLength = 100;
		if (dir.len() > maxDragLength)
		{
			drag.first = drag.second - dir.Norm() * maxDragLength;
		}
	}
	return true;
}

bool GameLayout::OnTap(float x, float y)
{
	std::vector<GC_Player*> players = _worldController.GetLocalPlayers();
	for (unsigned int playerIndex = 0; playerIndex != players.size(); ++playerIndex)
	{
		if( Controller *controller = _inputMgr.GetController(playerIndex) )
		{
			auto c2w = _gameViewHarness.CanvasToWorld(playerIndex, (int) x, (int) y);
			if (c2w.visible)
			{
				controller->OnTap(c2w.worldPos);
			}
		}
	}
	return true;
}

void GameLayout::OnChangeShowTime()
{
	_time->SetVisible(_conf.ui_showtime.Get());
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
