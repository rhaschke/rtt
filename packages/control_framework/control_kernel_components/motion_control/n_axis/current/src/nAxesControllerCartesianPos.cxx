// $Id: nAxisGeneratorCartesianPos.hpp,v 1.1.1.1 2003/12/02 20:32:06 kgadeyne Exp $
// Copyright (C) 2003 Klaas Gadeyne <klaas.gadeyne@mech.kuleuven.ac.be>
//                    Wim Meeussen  <wim.meeussen@mech.kuleuven.ac.be>
//  
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//  
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//  
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//  


#include "control_kernel/nAxesControllerCartesianPos.hpp"
#include <assert.h>


namespace ORO_ControlKernel
{

  nAxesControllerCartesianPos::nAxesControllerCartesianPos(std::string name)
    : nAxesControllerCartesianPos_typedef(name),
      _properties_read(false),
      _controller_gain("K", "Proportional Gain")
  {}


  nAxesControllerCartesianPos::~nAxesControllerCartesianPos(){};
  

  void nAxesControllerCartesianPos::pull()
  {
    // copy Input and Setpoint to local values
    _position_meas_DOI->Get(_position_meas_local);
    _position_desi_DOI->Get(_position_desi_local);
  }



  void nAxesControllerCartesianPos::calculate()
  {
    // feedback on position
    _velocity_out_local = diff(_position_meas_local, _position_desi_local);
    
    for(unsigned int i=0; i<6; i++)
      _velocity_out_local(i) *= _controller_gain.value()[i];
  }
  
  
  void nAxesControllerCartesianPos::push()      
  {
    _velocity_out_DOI->Set(_velocity_out_local.RefPoint( _position_meas_local.p * -1 ));
  }


  bool nAxesControllerCartesianPos::componentLoaded()
  {
    // get interface to Output data types
    if ( !Output->dObj()->Get("Twist", _velocity_out_DOI) ){
      cerr << "nAxesControllerCartesianPos::componentLoaded() DataObjectInterface not found" << endl;
      return false;
    }

    // set empty values
    ORO_Geometry::Twist _temp_twist;
    _velocity_out_DOI->Set(_temp_twist);

    return true;
  }


  bool nAxesControllerCartesianPos::componentStartup()
  {
    // check if updateProperties has been called
    if (!_properties_read){
      cerr << "nAxesControllerCartesianPos::componentStartup() Properties have not been read." << endl;
      return false;
    }

    // get interface to Input/Setpoint data types
    if ( !Input->dObj(   )->Get("Frame", _position_meas_DOI) ||
	 !SetPoint->dObj()->Get("Frame", _position_desi_DOI) ){
      cerr << "nAxesControllerCartesianPos::componentStartup() DataObjectInterface not found" << endl;
      return false;
    }
    return true;
  }
  

  bool nAxesControllerCartesianPos::updateProperties(const PropertyBag& bag)
  {
    // properties have been read
    _properties_read = true;

    // get properties
    if (!composeProperty(bag, _controller_gain) ){
      cerr << "nAxesControllerCartesianPos::updateProperties() failed" << endl;
      return false;
    }

    // check size of properties
    assert(_controller_gain.value().size() == 6);

    return true;
  }

  void nAxesControllerCartesianPos::exportProperties(ORO_CoreLib::PropertyBag& bag)
  {
    bag.add(&_controller_gain);
  }

  

} // namespace
