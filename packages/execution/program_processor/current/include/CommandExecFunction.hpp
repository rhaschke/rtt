/***************************************************************************
  tag: Peter Soetens  Tue Dec 21 22:43:07 CET 2004  CommandExecFunction.hpp 

                        CommandExecFunction.hpp -  description
                           -------------------
    begin                : Tue December 21 2004
    copyright            : (C) 2004 Peter Soetens
    email                : peter.soetens@mech.kuleuven.ac.be
 
 ***************************************************************************
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2.1 of the License, or (at your option) any later version.    *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/
 
 
#ifndef COMMAND_EXEC_FUNCTION_HPP
#define COMMAND_EXEC_FUNCTION_HPP

#include <corelib/ConditionInterface.hpp>
#include <corelib/CommandInterface.hpp>
#include "ProgramInterface.hpp"
#include "ProgramProcessor.hpp"
#include "DataSource.hpp"
#include <boost/shared_ptr.hpp>

namespace ORO_Execution
{
    /**
     * A condition which checks if a CommandExecFunction is done or not.
     */
    class ConditionExecFunction
        : public ORO_CoreLib::ConditionInterface
    {
        DataSource<ProgramInterface*>::shared_ptr _v;
    public:
        ConditionExecFunction( DataSource<ProgramInterface*>* v)
            : _v( v )
        {}

        bool evaluate()
        {
            return _v->get()->isStopped();
        }

        ORO_CoreLib::ConditionInterface* clone() const
        {
            return new ConditionExecFunction( _v.get() );
        }

        ORO_CoreLib::ConditionInterface* copy( std::map<const DataSourceBase*, DataSourceBase*>& alreadyCloned ) const
        {
            // after *all* the copying is done, _v will be set to the correct function
            // by the Command's copy.
            return new ConditionExecFunction( _v->copy( alreadyCloned ) );
        }

    };

    /**
     * A command which queues (dispatches) a FunctionFraph for execution
     * in a ProgramProcessor. See ConditionExecFunction to check if
     * it is done or not. It does not need the CommandDispatch
     * to be thread-safe.
     */
    class CommandExecFunction
        : public ORO_CoreLib::CommandInterface
    {
        ProgramProcessor* _proc;
        AssignableDataSource<ProgramInterface*>::shared_ptr _v;
        boost::shared_ptr<ProgramInterface> _foo;
        bool isqueued;
    public:
        CommandExecFunction( boost::shared_ptr<ProgramInterface> foo, ProgramProcessor* p, AssignableDataSource<ProgramInterface*>* v = 0 )
            : _proc(p), _v( v==0 ? new detail::VariableDataSource<ProgramInterface*>(foo.get()) : v ),  _foo( foo ), isqueued(false)
        {
        }

        ~CommandExecFunction() {
            //remove it before destruction.
            _proc->removeFunction( _foo.get() );
        }

        bool execute()
        {
            // this is asyn behaviour :
            if (isqueued == false ) {
                isqueued = true;
                return _proc->runFunction( _foo.get() );
            }
            // if it was queued already return if it is
            // in error or not.
            return ! _foo->inError();
        }
        void reset()
        {
            // reset the program, so that it is valid to be re-queued again
            _foo->reset();
            isqueued = false;
        }

        /**
         * Create a condition which checks if this command is finished or not.
         */
        ORO_CoreLib::ConditionInterface* createCondition()
        {
            return new ConditionExecFunction( _v.get() );
        }
        
        ORO_CoreLib::CommandInterface* clone() const
        {
            // _v is shared_ptr, so don't clone.
            return new CommandExecFunction( _foo, _proc, _v.get() );
        }
        
        ORO_CoreLib::CommandInterface* copy( std::map<const DataSourceBase*, DataSourceBase*>& alreadyCloned ) const
        {
            // this may seem strange, but :
            // make a copy of foo (a function), make a copy of _v (a datasource), store pointer to new foo in _v !
            boost::shared_ptr<ProgramInterface> fcpy( _foo->copy(alreadyCloned) );
            AssignableDataSource<ProgramInterface*>* vcpy = _v->copy(alreadyCloned);
            vcpy->set( fcpy.get() ); // since we own _foo, we may manipulate the copy of _v
            return new CommandExecFunction( fcpy , _proc, vcpy );
        }
        
    };

}

#endif
