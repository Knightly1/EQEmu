/*  EQEMu:  Everquest Server Emulator
	Copyright (C) 2001-2003  EQEMu Development Team (http://eqemulator.net)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.
  
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
	
	  You should have received a copy of the GNU General Public License
	  along with this program; if not, write to the Free Software
	  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifdef EMBPERL 

#include "masterentity.h" 

#include "EXTERN.h" 
#include "perl.h" 
#include "XSUB.h" 
#include "embxs.h" 

extern Database database; 

const char *getItemName(unsigned itemid) 
{ 
  const Item_Struct* item = NULL; 
  item = database.GetItem(itemid); 

  if (item) 
    return item->Name; 
  else 
    return NULL; 
} 

XS(XS_qc_getItemName); /* prototype to pass -Wmissing-prototypes */ 
XS(XS_qc_getItemName) 
{ 
    dXSARGS; 
    if (items != 1) 
        Perl_croak(aTHX_ "Usage: quest::getItemName(itemid)"); 
    { 
        unsigned        itemid = (unsigned)SvUV(ST(0)); 
        const char *    RETVAL; 
        dXSTARG; 
    RETVAL = getItemName(itemid); 
        sv_setpv(TARG, RETVAL); XSprePUSH; PUSHTARG; 
    } 
    XSRETURN(1); 
} 


EXTERN_C XS(boot_qc); /* prototype to pass -Wmissing-prototypes */ 
EXTERN_C XS(boot_qc) 
{ 
    dXSARGS; 
    char* file = __FILE__; 

    XS_VERSION_BOOTCHECK ; 

        newXS("quest::getItemName", XS_qc_getItemName, file); 

    XSRETURN_YES; 
} 

#endif // EMBPERL