/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (c) 2007 Gerhard Reitmayr <gerhard.reitmayr@gmail.com>
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

namespace VDR
{

// Simple template-based finite state machine. Based on FSM by Gerhard Reitmayr
// <https://github.com/GerhardR/fsm>


/*
 * State List
 *
 * Instead of storing the list of states as an enum and switching on its value
 * (a common FSM strategy), the FSM's states are defined using a linked list of
 * template classes. The link head is the state, and the link tail is a
 * template class containing the next state. The type list is expanded
 * recursively to select the appropriate state function.
 *
 * The state type can be typedef'd using a simple macro. For example, an ON/OFF
 * FSM would define the state type as:
 *
 * typedef STATELIST2(OFF, ON) StateType;
 */

template <int HEAD, class TAIL> struct StateListItem
{
  enum { Head = HEAD }; // The value of the state
  typedef TAIL Tail;    // The next state (hint: its value is TAIL::Head)
};

// State list terminator
struct StateListEnd { };

#define STATELIST1(a)                    StateListItem<a,StateListEnd>
#define STATELIST2(a,b)                  StateListItem<a,STATELIST1(b)>
#define STATELIST3(a,b,c)                StateListItem<a,STATELIST2(b,c)>
#define STATELIST4(a,b,c,d)              StateListItem<a,STATELIST3(b,c,d)>
#define STATELIST5(a,b,c,d,e)            StateListItem<a,STATELIST4(b,c,d,e)>
#define STATELIST6(a,b,c,d,e,f)          StateListItem<a,STATELIST5(b,c,d,e,f)>
#define STATELIST7(a,b,c,d,e,f,g)        StateListItem<a,STATELIST6(b,c,d,e,f,g)>
#define STATELIST8(a,b,c,d,e,f,g,h)      StateListItem<a,STATELIST7(b,c,d,e,f,g,h)>
#define STATELIST9(a,b,c,d,e,f,g,h,i)    StateListItem<a,STATELIST8(b,c,d,e,f,g,h,i)>
#define STATELIST10(a,b,c,d,e,f,g,h,i,j) StateListItem<a,STATELIST9(b,c,d,e,f,g,h,i,j)>

/*
 * Switch Implementation
 *
 * The linked list expansion happens here. We iterate over the type list until
 * we find the desired state.
 */
template <class STATELIST>
struct SwitchTemplate
{
  // CONTEXT is the StateMachine functor (CallEnter, CallExit, etc)
  template <class CONTEXT>
  static typename CONTEXT::ReturnType Work(int state, CONTEXT& context)
  {
    if (STATELIST::Head == state)
    {
      // Found our state, evaluate the specialisation for this state
      return context.template operator()<STATELIST::Head>();
    }
    else
    {
      // Recurse into the state list, expanding Tail until its Head matches our state
      return SwitchTemplate<typename STATELIST::Tail>::Work(state, context);
    }
  }
};

// Base case for template recursion above
template <>
struct SwitchTemplate<StateListEnd>
{
  template <class CONTEXT>
  static typename CONTEXT::ReturnType Work(int state, CONTEXT& context) { }
};


/*
 * Finite State Machine
 *
 * The actual state machine implementation. Embedded structs are functors
 * passed to the worker class to execute the right template specialisation of
 * the context object.
 */
template <class CONTEXT, class STATETYPE = typename CONTEXT::StateType>
class cFiniteStateMachine
{
private:
  CONTEXT& m_context;
  int      m_state;

  // Functor to execute Enter() template specialisation of new state
  struct CallEnter
  {
    typedef void ReturnType;
    CONTEXT& context;
    template <int STATE> ReturnType operator()() { return context.template Enter<STATE>(); }
  };

  // Functor to execute Exit() template specialisation of old state
  struct CallExit
  {
    typedef void ReturnType;
    CONTEXT& context;
    template <int STATE> ReturnType operator()() { return context.template Exit<STATE>(); }
  };

  // Functor to execute Event() template specialisation with no event data
  template <class RET>
  struct CallEventNoData
  {
    typedef RET ReturnType;
    CONTEXT& context;
    template <int STATE> ReturnType operator()() { return context.template Event<STATE>(); }
  };

  // Functor to execute Event() template specialisation with event data
  template <class RET, class DATA>
  struct CallEvent
  {
    typedef RET ReturnType;
    CONTEXT& context;
    DATA& data;
    template <int STATE> ReturnType operator()() { return context.template Event<STATE>(data); }
  };

  // Functor to execute Event() template specialisation with const event data
  template <class RET, class DATA>
  struct CallEventConst
  {
    typedef RET ReturnType;
    CONTEXT& context;
    const DATA& data;
    template <int STATE> ReturnType operator()() { return context.template Event<STATE>(data); }
  };

public:
  cFiniteStateMachine(CONTEXT& context) : m_context(context), m_state(STATETYPE::Head)
  {
    CallEnter cee = { m_context };
    SwitchTemplate<STATETYPE>::Work(m_state, cee);
  }

  int State() const { return m_state; }

  // Call enter/exit functors, no effect if state is unchanged
  void ChangeState(const int newState)
  {
    if (m_state != newState)
    {
      // Exit the current state
      CallExit cl = { m_context };
      SwitchTemplate<STATETYPE>::Work(m_state, cl);

      // Record the new state
      m_state = newState;

      // Enter the new state
      CallEnter cee = { m_context };
      SwitchTemplate<STATETYPE>::Work(m_state, cee);
    }
  }

  // Calls Event() for current state with no event data
  void Work()
  {
    // Use our switch statement implementation to select the correct Event() specialisation
    CallEventNoData<int> ce = { m_context };
    int newState = SwitchTemplate<STATETYPE>::Work(m_state, ce);

    // Call enter/exit functors if the state changed
    ChangeState(newState);
  }

  // Calls Event() for current state with extra event data
  template <class EVENT>
  void Work(const EVENT& ev)
  {
    // Use our switch statement implementation to select the correct Event() specialisation
    CallEventConst<int, EVENT> ce = { m_context, ev };
    int newState = SwitchTemplate<STATETYPE>::Work(m_state, ce);

    // Call enter/exit functors if the state changed
    ChangeState(newState);
  }
};

/*
 * Putting it all together, we can define a sample FSM:
 *
 * enum States { OFF, ON };
 *
 * struct LightSwitch
 * {
 *   // Define state list
 *   typedef STATELIST2(OFF, ON) StateType;
 *
 *   // Default template versions
 *   template <int> int  Event() { esyslog("Undefined event handler"); return OFF; }
 *   template <int> void Enter() { }
 *   template <int> void Exit()  { }
 * };
 *
 * // Specialisations for states that require behaviour
 * template <> int  LightSwitch::Event<ON>()  { dsyslog("Received push"); return OFF; };
 * template <> int  LightSwitch::Event<OFF>() { dsyslog("Received push"); return ON; };
 * template <> void LightSwitch::Enter<ON>()  { dsyslog("Enter ON - turn on the light"); }
 * template <> void LightSwitch::Enter<OFF>() { dsyslog("Enter OFF - turn off the light"); }
 * template <> void LightSwitch::Exit<ON>()   { dsyslog("Exit ON - do nothing"); }
 * template <> void LightSwitch::Exit<OFF>()  { dsyslog("Exit OFF - do nothing"); }
 *
 * int main()
 * {
 *   LightSwitch c;
 *   cStateMachine<LightSwitch> sm(c);
 *
 *   sm.Work(); // Turn light on
 *   sm.Work(); // Turn light off
 *   sm.Work(); // Turn light on
 * }
 */
}
