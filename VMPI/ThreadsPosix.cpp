/* ***** BEGIN LICENSE BLOCK *****
* Version: MPL 1.1/GPL 2.0/LGPL 2.1
*
* The contents of this file are subject to the Mozilla Public License Version
* 1.1 (the "License"); you may not use this file except in compliance with
* the License. You may obtain a copy of the License at
* http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
* for the specific language governing rights and limitations under the
* License.
*
* The Original Code is [Open Source Virtual Machine.].
*
* The Initial Developer of the Original Code is
* Adobe System Incorporated.
* Portions created by the Initial Developer are Copyright (C) 2004-2006
* the Initial Developer. All Rights Reserved.
*
* Contributor(s):
*   Adobe AS3 Team
*
* Alternatively, the contents of this file may be used under the terms of
* either the GNU General Public License Version 2 or later (the "GPL"), or
* the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
* in which case the provisions of the GPL or the LGPL are applicable instead
* of those above. If you wish to allow use of your version of this file only
* under the terms of either the GPL or the LGPL, and not to allow others to
* use your version of this file under the terms of the MPL, indicate your
* decision by deleting the provisions above and replace them with the notice
* and other provisions required by the GPL or the LGPL. If you do not delete
* the provisions above, a recipient may use your version of this file under
* the terms of any one of the MPL, the GPL or the LGPL.
*
* ***** END LICENSE BLOCK ***** */

#include "MMgc.h"

#ifdef MMGC_HAVE_PTHREAD_H

	#include <pthread.h>

	bool VMPI_tlsCreate(uintptr_t* tlsId)
	{
		pthread_key_t key;
		const int r = pthread_key_create(&key, NULL);
		
		if(r == 0)
		{
			// we expect the value to default to zero
			GCAssert(pthread_getspecific(key) == 0);
			*tlsId = (uintptr_t) key;
			return true;
		}

		return false;
	}

	void VMPI_tlsDestroy(uintptr_t tlsId)
	{
		pthread_key_delete((pthread_key_t)tlsId);
	}

	bool VMPI_tlsSetValue(uintptr_t tlsId, void* value)
	{
		const int r = pthread_setspecific((pthread_key_t)tlsId, value);
		GCAssert(pthread_getspecific((pthread_key_t)tlsId) == value);
		return (r == 0);
	}

	void* VMPI_tlsGetValue(uintptr_t tlsId)
	{
		return pthread_getspecific((pthread_key_t)tlsId);
	}

#else

	bool VMPI_tlsCreate(uintptr_t* tlsId)
	{
		intptr_t* id = (intptr_t*) malloc(sizeof(intptr_t*));
		*tlsId = (uintptr_t)id;
		
		return id != NULL;
	}

	void VMPI_tlsDestroy(uintptr_t tlsId)
	{
		free((intptr_t*)tlsId);
	}

	bool VMPI_tlsSetValue(uintptr_t tlsId, void* value)
	{
		*((intptr_t*)tlsId) = (intptr_t)value;
		return true;
	}

	void* VMPI_tlsGetValue(uintptr_t tlsId)
	{
		return (void*) *((intptr_t*)tlsId);
	}

#endif //MMGC_HAVE_PTHREAD_H
