#include "execution/GlobalCommandFactory.hpp"
#include "execution/CommandC.hpp"
#include "execution/CommandDispatch.hpp"
#include "execution/TryCommand.hpp"
#include "execution/ConditionComposite.hpp"
#include "execution/ConditionBoolDataSource.hpp"
#include "execution/FactoryExceptions.hpp"
#include <vector>

namespace ORO_Execution
{
    
    class CommandC::D
    {
    public:
        const GlobalCommandFactory* mgcf;
        std::string mobject, mname;
        ComCon comcon;
        std::vector<DataSourceBase::shared_ptr> args;
        bool masyn;
        int ticket;
        bool created() {
            return comcon.first != 0;
        }
        void checkAndCreate() {
            size_t sz = mgcf->getObjectFactory(mobject)->getArgumentList(mname).size();
            if ( created() )
                throw wrong_number_of_args_exception( sz, sz + 1 );
            if ( mgcf->hasCommand(mobject, mname) && sz == args.size() ) {
                // may throw.
                comcon = mgcf->getObjectFactory(mobject)->create(mname, args, false );
                // command: dispatch a try of the command, collect results to evaluate condition.
                TryCommand* tc = new TryCommand( comcon.first );
                CommandInterface* com = new CommandDispatch( mgcf->getCommandProcessor(), tc, tc->result().get() );
                // condition: if executed() and result() then evaluate condition().
                ConditionInterface* conb =  new ConditionCompositeAND( new ConditionBoolDataSource( tc->executed().get() ), 
                                                                       new TryCommandResult( tc->result(), false ));
                conb = new ConditionCompositeAND( conb, comcon.second );
                comcon.first = com;
                comcon.second = conb;
                args.clear();
            }
        }

    public:
        void newarg(DataSourceBase::shared_ptr na)
        {
            this->args.push_back( na );
            this->checkAndCreate();
        }

        D( const GlobalCommandFactory* gcf, const std::string& obj, const std::string& name, bool asyn)
            : mgcf(gcf), mobject(obj), mname(name), masyn(asyn), ticket(0)
        {
            comcon.first = 0;
            comcon.second = 0;
            this->checkAndCreate();
        }

        D(const D& other)
            : mgcf( other.mgcf), mobject(other.mobject), mname(other.mname),
              args( other.args ), masyn(other.masyn),
              ticket(other.ticket)
        {
            comcon.first = other.comcon.first->clone();
            comcon.second = other.comcon.second->clone();
        }

        ~D()
        {
            delete comcon.first;
            delete comcon.second;
        }

        bool execute() {
            if ( comcon.first && ticket == 0 ) {
                ticket = mgcf->getCommandProcessor()->process( comcon.first );
                return ticket != 0;
            }
            return false;
        }

        bool evaluate() {
            if (ticket !=0 && mgcf->getCommandProcessor()->isProcessed(ticket) )
                return comcon.second->evaluate();
            return false;
        }

        void reset()
        {
            ticket = 0;
            comcon.first->reset();
            comcon.second->reset();
        }
    };

    CommandC::CommandC()
    : d(0), cc()
    {
    }

    CommandC::CommandC(const GlobalCommandFactory* gcf, const std::string& obj, const std::string& name, bool asyn)
        : d( new D( gcf, obj, name, asyn) ), cc()
    {
        if ( d->comcon.first ) {
            this->cc.first = d->comcon.first->clone();
            this->cc.second = d->comcon.second->clone();
            delete d;
            d = 0;
        }
    }

    CommandC::CommandC(const CommandC& other)
        : d( other.d ? new D(*other.d) : 0 ), cc()
    {
        if (other.cc.first)
            this->cc.first = other.cc.first->clone();
        if (other.cc.second)
            this->cc.second = other.cc.second->clone();
    }

    CommandC::~CommandC()
    {
        delete d;
        delete cc.first;
        delete cc.second;
    }

    CommandC& CommandC::arg( DataSourceBase::shared_ptr a )
    {
        if (d)
            d->newarg( a );
        if ( d && d->comcon.first ) {
            this->cc.first = d->comcon.first->clone();
            this->cc.second = d->comcon.second->clone();
            delete d;
            d = 0;
        }
        return *this;
    }

    bool CommandC::execute() {
        // execute dispatch command
        if (cc.first)
            return cc.first->execute();
        return false;
    }

    bool CommandC::accepted() {
        // check if dispatch command was accepted.
        if (cc.first)
            return static_cast<CommandDispatch*>(cc.first)->accepted();
        return false;
    }

    bool CommandC::valid() {
        // check if dispatch command had valid args.
        if (cc.first)
            return static_cast<CommandDispatch*>(cc.first)->result();
        return false;
    }

    bool CommandC::evaluate() {
        // check if done
        if (cc.second )
            return cc.second->evaluate();
        return false;
    }

    void CommandC::reset()
    {
        if (cc.first)
            cc.first->reset();
        if (cc.second)
            cc.second->reset();
    }
}
