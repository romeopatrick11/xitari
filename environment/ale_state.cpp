/* *****************************************************************************
 * Xitari
 *
 * Copyright 2014 Google Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * *****************************************************************************
 * A.L.E (Arcade Learning Environment)
 * Copyright (c) 2009-2013 by Yavar Naddaf, Joel Veness, Marc G. Bellemare and 
 *   the Reinforcement Learning and Artificial Intelligence Laboratory
 * Released under the GNU General Public License; see License.txt for details. 
 *
 * Based on: Stella  --  "An Atari 2600 VCS Emulator"
 * Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
 *
 * *****************************************************************************
 */
#include "ale_state.hpp"
#include "emucore/m6502/src/System.hxx"
#include "emucore/Event.hxx"
#include "emucore/OSystem.hxx"
#include "emucore/Deserializer.hxx"
#include "games/RomSettings.hpp"
#include "common/Constants.h"
#include "archive_binary_in.hpp"
#include "archive_binary_out.hpp"


using namespace ale;

/** Default constructor - loads settings from system */ 
ALEState::ALEState():
  m_left_paddle(PADDLE_DEFAULT_VALUE),
  m_right_paddle(PADDLE_DEFAULT_VALUE),
  m_frame_number(0),
  m_episode_frame_number(0)
{
}

ALEState::ALEState(const ALEState &rhs, const std::string &serialized):
  m_left_paddle(rhs.m_left_paddle),
  m_right_paddle(rhs.m_right_paddle),
  m_frame_number(rhs.m_frame_number),
  m_episode_frame_number(rhs.m_episode_frame_number),
  m_serialized_state(serialized)
{
}

/** Restores ALE to the given previously saved state. */ 
void ALEState::load(OSystem* osystem, RomSettings* settings, const std::string &md5, const ALEState &rhs) {
  assert(rhs.m_serialized_state.length() > 0);
  
  // Deserialize the stored std::string into the emulator state
  Deserializer deser(rhs.m_serialized_state);
  
  osystem->console().system().loadState(md5, deser);
  settings->loadState(deser);
 
  // Copy over other member variables
  m_left_paddle = rhs.m_left_paddle; 
  m_right_paddle = rhs.m_right_paddle; 
  m_frame_number = rhs.m_frame_number; 
  m_episode_frame_number = rhs.m_episode_frame_number; 
}

ALEState ALEState::save(OSystem* osystem, RomSettings* settings, const std::string &md5) {
  // Use the emulator's built-in serialization to save the state
  Serializer ser;
  
  osystem->console().system().saveState(md5, ser);
  settings->saveState(ser);

  // Now make a copy of this state, also storing the emulator serialization
  return ALEState(*this, ser.get_str());
}

/* ***************************************************************************
 *  Calculates the Paddle resistance, based on the given x val
 * ***************************************************************************/
int ALEState::calcPaddleResistance(int x_val) {
  return x_val;  // this is different from the original stella implemebtation
}

void ALEState::incrementFrame(int numSteps) {
    m_frame_number += numSteps;
    m_episode_frame_number += numSteps;
}

void ALEState::resetVariables(Event * event) {
    m_episode_frame_number = 0;
    resetPaddles(event);
}

void ALEState::resetPaddles(Event * event) {
  setPaddles(event, PADDLE_DEFAULT_VALUE, PADDLE_DEFAULT_VALUE);
}

void ALEState::setPaddles(Event * event, int left, int right) {
  m_left_paddle = left; 
  m_right_paddle = right;

  // Compute the "resistance" (this is for vestigal clarity) 
  int left_resistance = calcPaddleResistance(m_left_paddle);
  int right_resistance = calcPaddleResistance(m_right_paddle);
  
  // Update the events with the new resistances
  event->set(Event::PaddleZeroResistance, left_resistance);
  event->set(Event::PaddleOneResistance, right_resistance);
}

/* *********************************************************************
 *  Updates the positions of the paddles, and sets an event for 
 *  updating the corresponding paddle's resistance
 * ********************************************************************/
 void ALEState::updatePaddlePositions(Event* event, int delta_left, int delta_right) {
    // Cap paddle outputs 

    m_left_paddle += delta_left;
    if (m_left_paddle < PADDLE_MIN) {
        m_left_paddle = PADDLE_MIN;
    } 
    if (m_left_paddle > PADDLE_MAX) {
        m_left_paddle = PADDLE_MAX;
    }
    
    m_right_paddle += delta_right;
    if (m_right_paddle < PADDLE_MIN) {
        m_right_paddle = PADDLE_MIN;
    } 
    if (m_right_paddle > PADDLE_MAX) {
        m_right_paddle = PADDLE_MAX;
    }
    
    // Now set the paddle to their new value
    setPaddles(event, m_left_paddle, m_right_paddle);
}


void ALEState::applyActionPaddles(Event* event, int player_a_action, int player_b_action) {
  // Reset keys
  resetKeys(event);

  // First compute whether we should increase or decrease the paddle position
  //  (for both left and right players)
  int delta_left;
  int delta_right;

    switch(player_a_action) {
        case PLAYER_A_RIGHT: 
        case PLAYER_A_RIGHTFIRE: 
        case PLAYER_A_UPRIGHT: 
        case PLAYER_A_DOWNRIGHT: 
        case PLAYER_A_UPRIGHTFIRE: 
        case PLAYER_A_DOWNRIGHTFIRE: 
          delta_left = -PADDLE_DELTA;   
            break; 
            
        case PLAYER_A_LEFT:
        case PLAYER_A_LEFTFIRE: 
        case PLAYER_A_UPLEFT: 
        case PLAYER_A_DOWNLEFT: 
        case PLAYER_A_UPLEFTFIRE: 
        case PLAYER_A_DOWNLEFTFIRE: 
      delta_left = PADDLE_DELTA;
            break;
    default:
      delta_left = 0;
      break;
    }

    switch(player_b_action) {
        case PLAYER_B_RIGHT: 
        case PLAYER_B_RIGHTFIRE: 
        case PLAYER_B_UPRIGHT: 
        case PLAYER_B_DOWNRIGHT: 
        case PLAYER_B_UPRIGHTFIRE: 
        case PLAYER_B_DOWNRIGHTFIRE: 
          delta_right = -PADDLE_DELTA;
            break; 
            
        case PLAYER_B_LEFT:
        case PLAYER_B_LEFTFIRE: 
        case PLAYER_B_UPLEFT: 
        case PLAYER_B_DOWNLEFT: 
        case PLAYER_B_UPLEFTFIRE: 
        case PLAYER_B_DOWNLEFTFIRE: 
      delta_right = PADDLE_DELTA;
            break;
    default:
      delta_right = 0;
      break;
    }

  // Now update the paddle positions
  updatePaddlePositions(event, delta_left, delta_right);

  // Handle reset
  if (player_a_action == RESET || player_b_action == RESET) 
    event->set(Event::ConsoleReset, 1);
  else if (player_a_action == SELECT || player_b_action == SELECT)
    event->set(Event::ConsoleSelect, 1);

  // Now add the fire event 
  switch (player_a_action) {
        case PLAYER_A_FIRE: 
        case PLAYER_A_UPFIRE: 
        case PLAYER_A_RIGHTFIRE: 
        case PLAYER_A_LEFTFIRE: 
        case PLAYER_A_DOWNFIRE: 
        case PLAYER_A_UPRIGHTFIRE: 
        case PLAYER_A_UPLEFTFIRE: 
        case PLAYER_A_DOWNRIGHTFIRE: 
        case PLAYER_A_DOWNLEFTFIRE: 
            event->set(Event::PaddleZeroFire, 1);
            break;
    default:
      // Nothing
      break;
  }
  
  switch (player_b_action) {
        case PLAYER_B_FIRE: 
        case PLAYER_B_UPFIRE: 
        case PLAYER_B_RIGHTFIRE: 
        case PLAYER_B_LEFTFIRE: 
        case PLAYER_B_DOWNFIRE: 
        case PLAYER_B_UPRIGHTFIRE: 
        case PLAYER_B_UPLEFTFIRE: 
        case PLAYER_B_DOWNRIGHTFIRE: 
        case PLAYER_B_DOWNLEFTFIRE: 
            event->set(Event::PaddleOneFire, 1);
            break;
    default:
      // Nothing
      break;
  }
}

void ALEState::setActionJoysticks(Event* event, int player_a_action, int player_b_action) {
  // Reset keys
  resetKeys(event);

  switch(player_a_action) {
      case PLAYER_A_NOOP: 
          break; 
          
      case PLAYER_A_FIRE: 
          event->set(Event::JoystickZeroFire, 1);
          break; 
          
      case PLAYER_A_UP: 
          event->set(Event::JoystickZeroUp, 1);
          break; 
          
      case PLAYER_A_RIGHT: 
          event->set(Event::JoystickZeroRight, 1);
          break; 
          
      case PLAYER_A_LEFT: 
          event->set(Event::JoystickZeroLeft, 1);
          break; 
          
      case PLAYER_A_DOWN: 
          event->set(Event::JoystickZeroDown, 1);
          break; 
          
      case PLAYER_A_UPRIGHT: 
          event->set(Event::JoystickZeroUp, 1);
          event->set(Event::JoystickZeroRight, 1);
          break; 
          
      case PLAYER_A_UPLEFT: 
          event->set(Event::JoystickZeroUp, 1);
          event->set(Event::JoystickZeroLeft, 1);
          break; 
          
      case PLAYER_A_DOWNRIGHT: 
          event->set(Event::JoystickZeroDown, 1);
          event->set(Event::JoystickZeroRight, 1);
          break; 
          
      case PLAYER_A_DOWNLEFT: 
          event->set(Event::JoystickZeroDown, 1);
          event->set(Event::JoystickZeroLeft, 1);
          break; 
          
      case PLAYER_A_UPFIRE: 
          event->set(Event::JoystickZeroUp, 1);
          event->set(Event::JoystickZeroFire, 1);
          break; 
          
      case PLAYER_A_RIGHTFIRE: 
          event->set(Event::JoystickZeroRight, 1);
          event->set(Event::JoystickZeroFire, 1); 
          break; 
          
      case PLAYER_A_LEFTFIRE: 
          event->set(Event::JoystickZeroLeft, 1);
          event->set(Event::JoystickZeroFire, 1); 
          break; 
          
      case PLAYER_A_DOWNFIRE: 
          event->set(Event::JoystickZeroDown, 1);
          event->set(Event::JoystickZeroFire, 1);
          break; 
          
      case PLAYER_A_UPRIGHTFIRE: 
          event->set(Event::JoystickZeroUp, 1);
          event->set(Event::JoystickZeroRight, 1);
          event->set(Event::JoystickZeroFire, 1);
          break; 
          
      case PLAYER_A_UPLEFTFIRE: 
          event->set(Event::JoystickZeroUp, 1);
          event->set(Event::JoystickZeroLeft, 1);
          event->set(Event::JoystickZeroFire, 1); 
          break; 
          
      case PLAYER_A_DOWNRIGHTFIRE: 
          event->set(Event::JoystickZeroDown, 1);
          event->set(Event::JoystickZeroRight, 1);
          event->set(Event::JoystickZeroFire, 1); 
          break; 
          
      case PLAYER_A_DOWNLEFTFIRE: 
          event->set(Event::JoystickZeroDown, 1);
          event->set(Event::JoystickZeroLeft, 1);
          event->set(Event::JoystickZeroFire, 1);
          break; 
      case RESET:
          event->set(Event::ConsoleReset, 1);
          break;
      case SELECT:
          event->set(Event::ConsoleSelect, 1);
          break;
      default: 
          std::cerr << "Invalid Player A Action: " << player_a_action;
          exit(-1); 
      
  }

  switch(player_b_action) {
  case PLAYER_B_NOOP: 
          break; 
          
      case PLAYER_B_FIRE: 
          event->set(Event::JoystickOneFire, 1);
          break; 
          
      case PLAYER_B_UP: 
          event->set(Event::JoystickOneUp, 1);
          break; 
          
      case PLAYER_B_RIGHT: 
          event->set(Event::JoystickOneRight, 1);
          break; 
          
      case PLAYER_B_LEFT: 
          event->set(Event::JoystickOneLeft, 1);
          break; 
          
      case PLAYER_B_DOWN: 
          event->set(Event::JoystickOneDown, 1);
          break; 
          
      case PLAYER_B_UPRIGHT: 
          event->set(Event::JoystickOneUp, 1);
          event->set(Event::JoystickOneRight, 1);
          break; 
          
      case PLAYER_B_UPLEFT: 
          event->set(Event::JoystickOneUp, 1);
          event->set(Event::JoystickOneLeft, 1);
          break; 
          
      case PLAYER_B_DOWNRIGHT: 
          event->set(Event::JoystickOneDown, 1);
          event->set(Event::JoystickOneRight, 1);
          break; 
          
      case PLAYER_B_DOWNLEFT: 
          event->set(Event::JoystickOneDown, 1);
          event->set(Event::JoystickOneLeft, 1);
          break; 
          
      case PLAYER_B_UPFIRE: 
          event->set(Event::JoystickOneUp, 1);
          event->set(Event::JoystickOneFire, 1);
          break; 
          
      case PLAYER_B_RIGHTFIRE: 
          event->set(Event::JoystickOneRight, 1);
          event->set(Event::JoystickOneFire, 1); 
          break; 
          
      case PLAYER_B_LEFTFIRE: 
          event->set(Event::JoystickOneLeft, 1);
          event->set(Event::JoystickOneFire, 1); 
          break; 
          
      case PLAYER_B_DOWNFIRE: 
          event->set(Event::JoystickOneDown, 1);
          event->set(Event::JoystickOneFire, 1);
          break; 
          
      case PLAYER_B_UPRIGHTFIRE: 
          event->set(Event::JoystickOneUp, 1);
          event->set(Event::JoystickOneRight, 1);
          event->set(Event::JoystickOneFire, 1);
          break; 
          
      case PLAYER_B_UPLEFTFIRE: 
          event->set(Event::JoystickOneUp, 1);
          event->set(Event::JoystickOneLeft, 1);
          event->set(Event::JoystickOneFire, 1); 
          break; 
          
      case PLAYER_B_DOWNRIGHTFIRE: 
          event->set(Event::JoystickOneDown, 1);
          event->set(Event::JoystickOneRight, 1);
          event->set(Event::JoystickOneFire, 1); 
          break; 
          
      case PLAYER_B_DOWNLEFTFIRE: 
          event->set(Event::JoystickOneDown, 1);
          event->set(Event::JoystickOneLeft, 1);
          event->set(Event::JoystickOneFire, 1);
          break; 
      case RESET:
          event->set(Event::ConsoleReset, 1);
          break;
      case SELECT:
          event->set(Event::ConsoleSelect, 1);
          break;
      default: 
          std::cerr << "Invalid Player B Action: " << player_b_action << std::endl;
          exit(-1); 
  }
}

/* ***************************************************************************
    Function resetKeys 
    Unpresses all control-relavant keys
 * ***************************************************************************/
void ALEState::resetKeys(Event* event) {
    event->set(Event::ConsoleReset, 0);
    event->set(Event::ConsoleSelect, 0);
    event->set(Event::JoystickZeroFire, 0);
    event->set(Event::JoystickZeroUp, 0);
    event->set(Event::JoystickZeroDown, 0);
    event->set(Event::JoystickZeroRight, 0);
    event->set(Event::JoystickZeroLeft, 0);
    event->set(Event::JoystickOneFire, 0);
    event->set(Event::JoystickOneUp, 0);
    event->set(Event::JoystickOneDown, 0);
    event->set(Event::JoystickOneRight, 0);
    event->set(Event::JoystickOneLeft, 0);
    
    // also reset paddle fire
    event->set(Event::PaddleZeroFire, 0);
    event->set(Event::PaddleOneFire, 0);
}

bool ALEState::equals(ALEState &rhs) {
  return (rhs.m_serialized_state == this->m_serialized_state &&
    rhs.m_left_paddle == this->m_left_paddle &&
    rhs.m_right_paddle == this->m_right_paddle &&
    rhs.m_frame_number == this->m_frame_number &&
    rhs.m_episode_frame_number == this->m_episode_frame_number);
}


std::string ALEState::getStateAsString() const {
    ArchiveBinaryOut oss;

    oss % m_left_paddle
        % m_right_paddle
        % m_frame_number
        % m_episode_frame_number
        % m_serialized_state;

    return oss.data();
}


ALEState::ALEState(const std::string &ale_state_string) {
    ArchiveBinaryIn iss(ale_state_string);

    iss % m_left_paddle
        % m_right_paddle
        % m_frame_number
        % m_episode_frame_number
        % m_serialized_state;
}


