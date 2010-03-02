/*
    Copyright (c) 2007-2009 Cyrus Daboo. All rights reserved.
    
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

#include "CNewEventDialog.h"

#include "CCalendarPopup.h"
#include "CCalendarView.h"
#include "CErrorDialog.h"
#include "CICalendar.h"
#include "CMulberryApp.h"
#include "CNewComponentAlarm.h"
#include "CNewComponentAttendees.h"
#include "CNewComponentDetails.h"
#include "CNewComponentRepeat.h"
#include "CPreferences.h"
#include "CSDIFrame.h"
#include "CUnicodeUtils.h"
#include "CXStringResources.h"

#include "CCalendarStoreManager.h"
#include "CCalendarStoreNode.h"
#include "CITIPProcessor.h"

// ---------------------------------------------------------------------------
//	CNewEventDialog														  [public]
/**
	Default constructor */

CNewEventDialog::CNewEventDialog() :
	CNewComponentDialog(),
	mStatus(true)	
{
}


// ---------------------------------------------------------------------------
//	~CNewEventDialog														  [public]
/**
	Destructor */

CNewEventDialog::~CNewEventDialog()
{
}

#pragma mark -

BEGIN_MESSAGE_MAP(CNewEventDialog, CModelessDialog)
	//{{AFX_MSG_MAP(CNewEventDialog)
	ON_COMMAND(IDC_CALENDAR_NEWEVENT_SUMMARY, OnChangeSummary)
	ON_COMMAND(IDC_CALENDAR_NEWEVENT_CALENDARPOPUP, OnChangeCalendar)
	ON_COMMAND(IDC_CALENDAR_NEWEVENT_ORGANISEREDIT, OnOrganiserEdit)
	ON_NOTIFY(TCN_SELCHANGE, IDC_CALENDAR_NEWEVENT_TABS, OnSelChangeTabs)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CNewEventDialog::OnInitDialog()
{
	CNewComponentDialog::OnInitDialog();

	// Get UI items
	mStatus.SubclassDlgItem(IDC_CALENDAR_NEWEVENT_STATUS, this, IDI_POPUPBTN, 0, 0, 0, true, false);
	mStatus.SetMenu(IDR_NEWEVENT_STATUS_POPUP);
	mAvailability.SubclassDlgItem(IDC_CALENDAR_NEWEVENT_AVAILABILITY, this);

	return true;
}

void CNewEventDialog::InitPanels()
{
	// Load each panel for the tabs
	mPanels.push_back(new CNewComponentDetails);
	mTabs.AddPanel(mPanels.back());

	mPanels.push_back(new CNewComponentRepeat);
	mTabs.AddPanel(mPanels.back());

	mPanels.push_back(new CNewComponentAlarm);
	mTabs.AddPanel(mPanels.back());

	mPanels.push_back(new CNewComponentAttendees);
	mTabs.AddPanel(mPanels.back());
}

void CNewEventDialog::SetComponent(iCal::CICalendarComponentRecur& vcomponent, const iCal::CICalendarComponentExpanded* expanded)
{
	CNewComponentDialog::SetComponent(vcomponent, expanded);

	mStatus.SetValue(static_cast<iCal::CICalendarVEvent&>(vcomponent).GetStatus() + IDM_NEWEVENT_STATUS_NONE);

	mAvailability.SetCheck(vcomponent.GetTransparent());
}

void CNewEventDialog::GetComponent(iCal::CICalendarComponentRecur& vcomponent)
{
	CNewComponentDialog::GetComponent(vcomponent);
	
	static_cast<iCal::CICalendarVEvent&>(vcomponent).EditStatus(static_cast<iCal::EStatus_VEvent>(mStatus.GetValue() - IDM_NEWEVENT_STATUS_NONE));
	vcomponent.EditTransparent(mAvailability.GetCheck());
}

void CNewEventDialog::SetReadOnly(bool read_only)
{
	CNewComponentDialog::SetReadOnly(read_only);

	mStatus.EnableWindow(!mReadOnly);
	mAvailability.EnableWindow(!mReadOnly);
}

void CNewEventDialog::ChangedMyStatus(const iCal::CICalendarProperty& attendee, const cdstring& new_status)
{
	static_cast<CNewComponentAttendees*>(mPanels.back())->ChangedMyStatus(attendee, new_status);

	// Handle TRANSP
	mAvailability.SetCheck(new_status == iCal::cICalAttribute_PARTSTAT_DECLINED);
}

void CNewEventDialog::OnOrganiserEdit()
{
	SetReadOnly(mOrganiserEdit.GetCheck() == 0);
}

bool CNewEventDialog::DoNewOK()
{
	// Check and get new calendar if different
	iCal::CICalendarRef newcal;
	iCal::CICalendar* new_cal = NULL;
	if (!GetCalendar(0, newcal, new_cal))
		// Return to dialog
		return false;

	// Get updated info
	GetComponent(*mComponent);
	
	// Look for change to calendar
	if (newcal != mComponent->GetCalendar())
	{
		// Use new calendar
		mComponent->SetCalendar(newcal);
		
		// Set the default calendar for next time
		const calstore::CCalendarStoreNode* node = calstore::CCalendarStoreManager::sCalendarStoreManager->GetNode(new_cal);
		if (node != NULL)
			CPreferences::sPrefs->mDefaultCalendar.SetValue(node->GetAccountName());
	}

	// Add to calendar (this will do the display update)
	new_cal->AddNewVEvent(static_cast<iCal::CICalendarVEvent*>(mComponent));
	CCalendarView::EventChangedAll(static_cast<iCal::CICalendarVEvent*>(mComponent));
	
	return true;
}

bool CNewEventDialog::DoEditOK()
{
	// Find the original calendar if it still exists
	iCal::CICalendarRef oldcal = mComponent->GetCalendar();
	iCal::CICalendar* old_cal = iCal::CICalendar::GetICalendar(oldcal);
	if (old_cal == NULL)
	{
		// Inform user of missing calendar
		CErrorDialog::StopAlert(rsrc::GetString("CNewEventDialog::MissingOriginal"));
		
		// Disable OK button
		GetDlgItem(IDOK)->EnableWindow(false);
		
		return false;
	}
	
	// Find the original event if it still exists
	iCal::CICalendarVEvent*	original = static_cast<iCal::CICalendarVEvent*>(old_cal->FindComponent(mComponent));
	if (original == NULL)
	{
		// Inform user of missing calendar
		CErrorDialog::StopAlert(rsrc::GetString("CNewEventDialog::MissingOriginal"));
		
		// Disable OK button and return to dialog
		GetDlgItem(IDOK)->EnableWindow(false);
		return false;
	}

	// Check and get new calendar if different
	iCal::CICalendarRef newcal;
	iCal::CICalendar* new_cal = NULL;
	if (!GetCalendar(oldcal, newcal, new_cal))
		// Return to dialog
		return false;

	if (mRecurring)
	{
		CErrorDialog::EDialogResult result = CErrorDialog::PoseDialog(CErrorDialog::eErrDialog_Caution,
																	  "ErrorDialog::Btn::ChangeAllEvents",
																	  "ErrorDialog::Btn::ChangeThisEvent",
																	  "ErrorDialog::Btn::ChangeThisFutureEvent",
																	  "ErrorDialog::Btn::Cancel",
																	  "ErrorDialog::Text::ChangeRecurEvent", 4);

		// Cancel
		if (result == CErrorDialog::eBtn4)
			return false;

		// All Events
		if (result == CErrorDialog::eBtn1)
		{
			// Change the master and all overrides
			if (mIsOverride)
			{
			}
			else
			{
				// Get updated info into original event
				GetComponent(*original);

				// Do calendar change
				if (new_cal != NULL)
				{
					// Remove from old calendar (without deleting)
					old_cal->RemoveVEvent(original, false);

					// Add to new calendar (without initialising)
					original->SetCalendar(newcal);
					new_cal->AddNewVEvent(original, true);
				}

				// Tell it it has changed (i.e. bump sequence, dirty calendar)
				original->Changed();

				CCalendarView::EventChangedAll(dynamic_cast<iCal::CICalendarVEvent*>(mComponent));
			}
		}

		// This event
		else if (result == CErrorDialog::eBtn2)
		{
			// If override, just change it
			if (mIsOverride)
			{
				// Get updated info into original event
				GetComponent(*original);

				// Do calendar change
				if (new_cal != NULL)
				{
					// Remove from old calendar (without deleting)
					old_cal->RemoveVEvent(original, false);

					// Add to new calendar (without initialising)
					original->SetCalendar(newcal);
					new_cal->AddNewVEvent(original, true);
				}

				// Tell it it has changed (i.e. bump sequence, dirty calendar)
				original->Changed();

				CCalendarView::EventChangedAll(dynamic_cast<iCal::CICalendarVEvent*>(mComponent));
			}
			else
			{
				// Need to create an override for this instance. Use original instance start as RECURRENCE-ID
				iCal::CICalendarVEvent* new_override = new iCal::CICalendarVEvent(*original);
				iCal::CICalendarProperty rid(iCal::cICalProperty_RECURRENCE_ID, mExpanded->GetInstanceStart());
				new_override->AddProperty(rid);

				// Get updated info into new override event
				GetComponent(*new_override);

				// Strip recurrence properties
				new_override->RemoveProperties(iCal::cICalProperty_RRULE);
				new_override->RemoveProperties(iCal::cICalProperty_RDATE);
				new_override->RemoveProperties(iCal::cICalProperty_EXRULE);
				new_override->RemoveProperties(iCal::cICalProperty_EXDATE);

				new_override->InitDTSTAMP();
				new_override->Finalise();

				// Add to calendar (this will do the display update)
				(new_cal != NULL ? new_cal : old_cal)->AddNewVEvent(static_cast<iCal::CICalendarVEvent*>(new_override), true);
				CCalendarView::EventChangedAll(static_cast<iCal::CICalendarVEvent*>(new_override));
			}
		}

		// Future events
		else if (result == CErrorDialog::eBtn3)
		{
		}
	}
	else
	{
		// Get updated info into original event
		GetComponent(*original);

		// Do calendar change
		if (new_cal != NULL)
		{
			// Remove from old calendar (without deleting)
			old_cal->RemoveVEvent(original, false);

			// Add to new calendar (without initialising)
			original->SetCalendar(newcal);
			new_cal->AddNewVEvent(original, true);
		}
		
		// Tell it it has changed (i.e. bump sequence, dirty calendar)
		original->Changed();

		CCalendarView::EventChangedAll(dynamic_cast<iCal::CICalendarVEvent*>(mComponent));
	}
	
	return true;
}

void CNewEventDialog::DoCancel()
{
	// Delete the event which we own and is not going to be used
	delete mComponent;
	mComponent = NULL;
}

void CNewEventDialog::StartNew(const iCal::CICalendarDateTime& dtstart, const iCal::CICalendar* calin)
{
	const iCal::CICalendarList& cals = calstore::CCalendarStoreManager::sCalendarStoreManager->GetActiveCalendars();
	if (cals.size() == 0)
		return;

	const iCal::CICalendar* cal = calin;
	if (cal == NULL)
	{
		// Default calendar is the first one
		cal = cals.front();

		// Check prefs for default calendar
		const calstore::CCalendarStoreNode* node = calstore::CCalendarStoreManager::sCalendarStoreManager->GetNode(CPreferences::sPrefs->mDefaultCalendar.GetValue());
		if ((node != NULL) && (node->GetCalendar() != NULL))
			cal = node->GetCalendar();
	}

	// Start with an empty new event
	std::auto_ptr<iCal::CICalendarVEvent> vevent(static_cast<iCal::CICalendarVEvent*>(iCal::CICalendarVEvent::Create(cal->GetRef())));
	
	// Duration is one hour
	iCal::CICalendarDuration duration(60 * 60);
	
	// Set event with initial timing
	vevent->EditTiming(dtstart, duration);

	StartModeless(*vevent.release(), NULL, CNewEventDialog::eNew);
}

void CNewEventDialog::StartEdit(const iCal::CICalendarVEvent& original, const iCal::CICalendarComponentExpanded* expanded)
{
	// Look for an existinf dialog for this event
	for(std::set<CNewComponentDialog*>::const_iterator iter = sDialogs.begin(); iter != sDialogs.end(); iter++)
	{
		if ((*iter)->ContainsComponent(original))
		{
			FRAMEWORK_WINDOW_TO_TOP(*iter)
			return;
		}
	}

	// Use a copy of the event
	iCal::CICalendarVEvent* vevent = new iCal::CICalendarVEvent(original);
	
	StartModeless(*vevent, expanded, CNewEventDialog::eEdit);
}

void CNewEventDialog::StartDuplicate(const iCal::CICalendarVEvent& original)
{
	// Start with an empty new event
	iCal::CICalendarVEvent* vevent = new iCal::CICalendarVEvent(original);
	vevent->Duplicated();
	
	StartModeless(*vevent, NULL, CNewEventDialog::eDuplicate);
}

void CNewEventDialog::StartModeless(iCal::CICalendarVEvent& vevent, const iCal::CICalendarComponentExpanded* expanded, EModelessAction action)
{
	CNewEventDialog* dlog = new CNewEventDialog;
	dlog->Create(IDD_CALENDAR_NEWEVENT, CSDIFrame::GetAppTopWindow());
	dlog->SetAction(action);
	dlog->SetComponent(vevent, expanded);
	dlog->ShowWindow(SW_SHOW);
}
