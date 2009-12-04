/*
    Copyright (c) 2007 Cyrus Daboo. All rights reserved.
    
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    
        http://www.apache.org/licenses/LICENSE-2.0
    
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/


// CEditIdentitySecurity.h : header file
//

#ifndef __CEDITIDENTITYSECURITY__MULBERRY__
#define __CEDITIDENTITYSECURITY__MULBERRY__

#include "CTabPanel.h"

#include "HPopupMenu.h"

/////////////////////////////////////////////////////////////////////////////
// CEditIdentitySecurity dialog

class JXDownRect;
class JXTextCheckbox;
class CTextInputField;

class CEditIdentitySecurity : public CTabPanel
{
public:
	CEditIdentitySecurity(JXContainer* enclosure,
				const HSizingOption hSizing, const VSizingOption vSizing,
				const JCoordinate x, const JCoordinate y,
				const JCoordinate w, const JCoordinate h)
		: CTabPanel(enclosure, hSizing, vSizing, x, y, w, h) {}

	virtual void OnCreate();
	virtual void SetData(void* data);			// Set data
	virtual bool UpdateData(void* data);		// Force update of data

protected:
// begin JXLayout1

    JXDownRect*      mGroup;
    JXTextCheckbox*  mActive;
    JXTextCheckbox*  mSign;
    JXTextCheckbox*  mEncrypt;
    HPopupMenu*      mSignWithPopup;
    CTextInputField* mSignOther;

// end JXLayout1

	virtual void Receive(JBroadcaster* sender, const Message& message);

			void OnActive(bool active);
			void OnSignWithPopup(JIndex nID);
};

#endif
