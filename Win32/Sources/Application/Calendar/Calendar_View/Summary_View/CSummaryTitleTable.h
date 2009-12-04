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

#ifndef H_CSummaryTitleTable
#define H_CSummaryTitleTable
#pragma once

#include "CSimpleTitleTable.h"

// ===========================================================================
//	MonthTitleTable
class CSummaryTable;

class	CSummaryTitleTable : public CSimpleTitleTable
{
public:
						CSummaryTitleTable();
	virtual				~CSummaryTitleTable();

	void				SetTable(CSummaryTable* table)
	{
		mTable = table;
		TableChanged();
	}

	void				TableChanged();

protected:
	CSummaryTable*		mTable;

	afx_msg int 		OnCreate(LPCREATESTRUCT lpCreateStruct);

	DECLARE_MESSAGE_MAP()
};

#endif
