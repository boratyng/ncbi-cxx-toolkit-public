/*  $Id$
* ===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*
* Authors:  Paul Thiessen
*
* File Description:
*      Class definition for the Show/Hide dialog
*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.1  2000/11/17 19:48:14  thiessen
* working show/hide alignment row
*
* ===========================================================================
*/

#include "cn3d/show_hide_dialog.hpp"

USING_NCBI_SCOPE;


BEGIN_SCOPE(Cn3D)

BEGIN_EVENT_TABLE(ShowHideDialog, wxFrame)
    EVT_LISTBOX     (LISTBOX,   ShowHideDialog::OnSelection) 
    EVT_BUTTON      (B_APPLY,   ShowHideDialog::OnButton)
    EVT_BUTTON      (B_CANCEL,  ShowHideDialog::OnButton)
    EVT_BUTTON      (B_DONE,    ShowHideDialog::OnButton)
    EVT_CLOSE                  (ShowHideDialog::OnCloseWindow)
END_EVENT_TABLE()

ShowHideDialog::ShowHideDialog(
    const wxString items[],
    std::vector < bool >& itemsEnabled,
    ShowHideCallback *callback,
    wxWindow* parent,
    wxWindowID id,
    const wxString& title,
    const wxPoint& pos,
    const wxSize& size
) :
    wxFrame(parent, id, title, pos, size, wxCAPTION | wxRESIZE_BORDER | wxFRAME_FLOAT_ON_PARENT),
    callbackObject(callback)
{
    SetAutoLayout(true);
    SetSizeHints(150, 100);

    static const int margin = 0;

    wxButton *doneB = new wxButton(this, B_DONE, "Done");
    wxLayoutConstraints *c1 = new wxLayoutConstraints();
    c1->bottom.SameAs       (this,      wxBottom,   margin);
    c1->right.SameAs        (this,      wxRight,    margin);
    c1->width.PercentOf     (this,      wxWidth,    33);
    c1->height.AsIs         ();
    doneB->SetConstraints(c1);

    applyB = new wxButton(this, B_APPLY, "Apply");
    wxLayoutConstraints *c2 = new wxLayoutConstraints();
    c2->bottom.SameAs       (this,      wxBottom,   margin);
    c2->left.SameAs         (this,      wxLeft,     margin);
    c2->width.PercentOf     (this,      wxWidth,    33);
    c2->top.SameAs          (doneB,     wxTop);
    applyB->SetConstraints(c2);
    applyB->Enable(false);

    wxButton *cancelB = new wxButton(this, B_CANCEL, "Cancel");
    wxLayoutConstraints *c3 = new wxLayoutConstraints();
    c3->bottom.SameAs       (this,      wxBottom,   margin);
    c3->left.RightOf        (applyB,                margin);
    c3->right.LeftOf        (doneB,                 margin);
    c3->top.SameAs          (doneB,     wxTop);
    cancelB->SetConstraints(c3);

    listBox = new wxListBox(this, LISTBOX, wxDefaultPosition, wxDefaultSize,
        itemsEnabled.size(), items, wxLB_EXTENDED | wxLB_HSCROLL | wxLB_NEEDED_SB);
    wxLayoutConstraints *c4 = new wxLayoutConstraints();
    c4->top.SameAs          (this,      wxTop,      margin);
    c4->left.SameAs         (this,      wxLeft,     margin);
    c4->right.SameAs        (this,      wxRight,    margin);
    c4->bottom.Above        (doneB,                 margin);
    listBox->SetConstraints(c4);

    Layout();   // just to be safe; really only needed if this window isn't made resizable

    // set initial item selection (reverse order, so scroll bar is initially at the top)
    for (int i=itemsEnabled.size()-1; i>=0; i--)
        listBox->SetSelection(i, itemsEnabled[i]);
}

void ShowHideDialog::Activate(void)
{
    if (!callbackObject) {
        ERR_POST(Error << "ShowHideDialog must be created with non-null callback pointer");
        return;
    }

    dialogActive = true;
    Show(true);
    MakeModal(true);

    // enter the modal loop  (this code snippet borrowed from src/msw/dialog.cpp)
    while (dialogActive)
    {
#if wxUSE_THREADS
        wxMutexGuiLeaveOrEnter();
#endif // wxUSE_THREADS
        while (!wxTheApp->Pending() && wxTheApp->ProcessIdle()) ;
        // a message came or no more idle processing to do
        wxTheApp->DoMessage();
    }

    MakeModal(false);
    return;
}

void ShowHideDialog::EndEventLoop(void)
{
    dialogActive = false;
}

void ShowHideDialog::OnSelection(wxCommandEvent& event)
{
    applyB->Enable(true);
}

void ShowHideDialog::OnButton(wxCommandEvent& event)
{
    if (event.GetId() == B_CANCEL) {
        EndEventLoop();
    } 
    
    else if (event.GetId() == B_DONE || event.GetId() == B_APPLY) {

        // only do this if selection has changed since last time
        if (applyB->IsEnabled()) {
            // do the callback with the current selection state
            std::vector < bool > itemsEnabled(listBox->Number());
            for (int i=0; i<listBox->Number(); i++) 
                itemsEnabled[i] = listBox->Selected(i);
            callbackObject->SelectionCallback(itemsEnabled);
            applyB->Enable(false);
        }
        
        if (event.GetId() == B_DONE)
            EndEventLoop();
    }
}

void ShowHideDialog::OnCloseWindow(wxCommandEvent& event)
{
    EndEventLoop();
}

END_SCOPE(Cn3D)

