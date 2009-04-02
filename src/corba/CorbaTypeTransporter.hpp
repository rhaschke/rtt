#ifndef CORBA_TYPE_TRANSPORTER_H
#define CORBA_TYPE_TRANSPORTER_H

#include "DataFlowI.h"
#include "../TypeTransporter.hpp"

namespace RTT {
    class ChannelElementBase;
    class ConnPolicy;
    class InputPortInterface;

    namespace Corba {
	class CorbaTypeTransporter : public RTT::detail::TypeTransporter
	{
	public:
	    virtual ChannelElement_i* createChannelElement_i(PortableServer::POA_ptr poa) const = 0;
	    virtual ChannelElementBase* buildReaderHalf(RTT::InputPortInterface& reader,
		    RTT::ConnPolicy const& policy) const = 0;

	};

        template<typename T>
	class RemoteChannelElement 
	    : public ChannelElement_i
	    , public RTT::ChannelElement<T>
	{
	    typename RTT::ValueDataSource<T>::shared_ptr data_source;

	public:
	    RemoteChannelElement(CorbaTypeTransporter const& transport, PortableServer::POA_ptr poa)
		: ChannelElement_i(transport, poa)
		, data_source(new RTT::ValueDataSource<T>)
            {
                // CORBA refcount-managed servants must start with a refcount of
                // 1
                this->ref();
            }

            /** Increase the reference count, called from the CORBA side */
            void _add_ref()
            { this->ref(); }
            /** Decrease the reference count, called from the CORBA side */
            void _remove_ref()
            { this->deref(); }


            void remoteSignal()
            { RTT::ChannelElement<T>::signal(); }
            void signal() const
            { remote_side->remoteSignal(); }

            void remoteDisconnect(bool writer_to_reader)
            {
		RTT::ChannelElement<T>::disconnect(writer_to_reader);
                remote_side = 0;
		PortableServer::ObjectId_var oid=mpoa->servant_to_id(this);
		mpoa->deactivate_object(oid.in());
	    }

            void disconnect(bool writer_to_reader)
            {
		remote_side->remoteDisconnect(writer_to_reader);
		remote_side = 0;
		PortableServer::ObjectId_var oid=mpoa->servant_to_id(this);
		mpoa->deactivate_object(oid.in());
	    }

            bool read(typename RTT::ChannelElement<T>::reference_t sample)
            {
                ::CORBA::Any_var remote_value;
                if (remote_side->read(remote_value))
                {
                    transport.updateBlob(&remote_value.in(), data_source);
                    sample = data_source->rvalue();
                    return true;
                }
                else return false;
            }

            ::CORBA::Boolean read(::CORBA::Any_OUT_arg sample)
            {
                if (RTT::ChannelElement<T>::read(data_source->set()))
                {
                    sample = static_cast<CORBA::Any*>(transport.createBlob(data_source));
                    return true;
                }
                else return false;
            }

            bool write(typename RTT::ChannelElement<T>::param_t sample)
            {
                data_source->set(sample);
                CORBA::Any_var ret = static_cast<CORBA::Any*>(transport.createBlob(data_source));
                remote_side->write(ret.in());
		return true;
            }

            void write(const ::CORBA::Any& sample)
            {
                transport.updateBlob(&sample, data_source);
                RTT::ChannelElement<T>::write(data_source->rvalue());
            }

        };
    }
}

#endif

