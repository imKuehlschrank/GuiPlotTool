#include <TGClient.h>
#include <TCanvas.h>
#include <TF1.h>
#include <TRandom.h>
#include <TGButton.h>
#include <TGFrame.h>
#include <TRootEmbeddedCanvas.h>
#include <RQ_OBJECT.h>
#include "TStyle.h"


// To search for loading SiStrip files, for nice visuals:
//OnTrack__TID__PLUS__ring__
//InPixel

using namespace std;

enum EMyMessageTypes {
   M_FILE_OPEN,
   M_FILE_EXIT
};

class HistogramInfo {
public:
    HistogramInfo(TObject* o, string p) : obj(o), filePath(p) {}

    TObject* GetObj()  const { return obj; }
    string   GetPath() const { return filePath; }
    string   GetName() const { return obj->GetName(); }

private:
    TObject* obj;
    string filePath;
};

class MyMainFrame {
    RQ_OBJECT("MyMainFrame")

public:
    MyMainFrame(const TGWindow *p, UInt_t w, UInt_t h);
    virtual ~MyMainFrame();

    void LoadAllPlotsFromDir(TDirectory *src);
    void DisplayMapInListBox(const map<Int_t, HistogramInfo> &m, TGListBox *listbox);
    void FilterBySearchBox();

    void RemoveFromSelection(Int_t id);
    void AddToSelection(Int_t id);

    void ClearSelectionListbox();
    void PreviewSelection();
    void Superimpose();
    void MergeSelection();

    string LoadFileFromDialog();

    void ToggleEnableRenameTextbox();
    void ToggleXMaxTextbox();
    void ToggleYMaxTextbox();
    void SetStyle();


    void UpdateDistplayListboxes();
    void HandleMenu(Int_t a);

    void SetPublicationStyle(TStyle* style);

    void SetCheckboxOptions(TH1* elem);
    void CreateStatBoxes(vector<TH1*>& plots, vector<TPaveStats*>& statboxes);
    void DrawPlots(vector<TH1*>& plots, vector<TPaveStats*>& statboxes, string option="");

private:
    // GUI elements
    TGMenuBar*    fMenuBar;
    TGPopupMenu*  fMenuFile;
    TGFileDialog* loadDialog;

    TGMainFrame* fMain;
    TGLabel*     currdirLabel;

    TGTextEntry* searchBox;
    TGTextEntry* renameTextbox;

    TGNumberEntryField* xminNumbertextbox;
    TGNumberEntryField* xmaxNumbertextbox;
    TGNumberEntryField* yminNumbertextbox;
    TGNumberEntryField* ymaxNumbertextbox;

    TGListBox*   mainListBox;
    TGListBox*   selectionListBox;

    TGCheckButton* displayPathCheckBox;
    TGCheckButton* tdrstyleCheckBox;
    TGCheckButton* statsCheckBox;
    TGCheckButton* legendCheckBox;
    TGCheckButton* normalizeCheckBox; // Only when superimposing
    TGCheckButton* renameCheckbox;
    TGCheckButton* xRangeCheckbox;
    TGCheckButton* yRangeCheckbox;

    TCanvas* resultCanvas = nullptr;
    TStyle* current_style = nullptr;

    // File Stuff
    TFile*      file;
    TDirectory* baseDir;

    // Model
    map<Int_t, HistogramInfo> table;
    map<Int_t, HistogramInfo> selection;

    Int_t freeId = 0;
    Int_t GetNextFreeId() { return freeId++; }

    void ResetGuiElements();
    void InitAll();
    void InitStyles();
};

MyMainFrame::MyMainFrame(const TGWindow *p, UInt_t w, UInt_t h) {
    fMain = new TGMainFrame(p, w, h);

    // #### DESIGN ####

    // ---- Menu Bar
    fMenuBar = new TGMenuBar(fMain, 35, 50, kHorizontalFrame);

    fMenuFile = new TGPopupMenu(gClient->GetRoot());
    fMenuFile->AddEntry(" &Open...\tCtrl+O", M_FILE_OPEN, 0, gClient->GetPicture("bld_open.png"));
    fMenuFile->AddSeparator();
    fMenuFile->AddEntry(" E&xit\tCtrl+Q", M_FILE_EXIT, 0, gClient->GetPicture("bld_exit.png"));
    fMenuBar->AddPopup("&File", fMenuFile, new TGLayoutHints(kLHintsLeft, 0, 4, 0, 0));

    // ---- Top label
    currdirLabel = new TGLabel(fMain,"");
    currdirLabel->MoveResize(0,0,500,16);

    // ---- Quicksearch Frame
    TGGroupFrame* quicksearchFrame = new TGGroupFrame(fMain,"Quicksearch");

    searchBox = new TGTextEntry(quicksearchFrame);
    displayPathCheckBox = new TGCheckButton(quicksearchFrame,"Display path");

    quicksearchFrame->AddFrame(searchBox,           new TGLayoutHints(kLHintsExpandX ,2,2,2,2));
    quicksearchFrame->AddFrame(displayPathCheckBox, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

    // ---- Listboxes
    TGVerticalFrame* listboxesFrame = new TGVerticalFrame(fMain, 200, 40);

    TGHorizontalFrame* mainListboxFrame = new TGHorizontalFrame(listboxesFrame, 10, 250, kFixedHeight);
    TGHorizontalFrame* selectionListboxFrame = new TGHorizontalFrame(listboxesFrame);

    mainListBox      = new TGListBox(mainListboxFrame, -1, kSunkenFrame);
    selectionListBox = new TGListBox(selectionListboxFrame, -1, kSunkenFrame);

    mainListboxFrame->AddFrame(mainListBox,          new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
    selectionListboxFrame->AddFrame(selectionListBox,new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

    TGHSplitter *hsplitter = new TGHSplitter(listboxesFrame,2,2);
    hsplitter->SetFrame(mainListboxFrame, kTRUE);

    listboxesFrame->AddFrame(mainListboxFrame,      new TGLayoutHints(kLHintsExpandX, 5, 5, 3, 4));
    listboxesFrame->AddFrame(hsplitter,             new TGLayoutHints(kLHintsTop | kLHintsExpandX));
    listboxesFrame->AddFrame(selectionListboxFrame, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY , 5, 5, 3, 4));


    // --- All Controls
    TGHorizontalFrame* controlFrame = new TGHorizontalFrame(fMain, 200, 40);

        // ------- Selection Buttons
    TGVerticalFrame* selectionControlFrameButtons = new TGVerticalFrame(controlFrame, 200, 40);

    TGTextButton* clearSelectionButton   = new TGTextButton(selectionControlFrameButtons, "&Clear List");
    TGTextButton* previewSelectionButton = new TGTextButton(selectionControlFrameButtons, "&Preview List");

    selectionControlFrameButtons->AddFrame(clearSelectionButton,   new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    selectionControlFrameButtons->AddFrame(previewSelectionButton, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));

        // ------- Checkboxes
    TGVerticalFrame* controlFrameCheckboxes = new TGVerticalFrame(controlFrame, 200, 80);

    tdrstyleCheckBox  = new TGCheckButton(controlFrameCheckboxes, "Pub. Style");
    statsCheckBox     = new TGCheckButton(controlFrameCheckboxes, "Show Stats");
    legendCheckBox    = new TGCheckButton(controlFrameCheckboxes, "Show Legend");
    normalizeCheckBox = new TGCheckButton(controlFrameCheckboxes, "Normalize");

    controlFrameCheckboxes->AddFrame(tdrstyleCheckBox,  new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameCheckboxes->AddFrame(statsCheckBox,     new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameCheckboxes->AddFrame(legendCheckBox,    new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameCheckboxes->AddFrame(normalizeCheckBox, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));

        // ------- Output Buttons
    TGVerticalFrame* outputControlFrameButtons = new TGVerticalFrame(controlFrame, 200, 40);

    TGTextButton* mergeSelectionButton    = new TGTextButton(outputControlFrameButtons, "&Merge Only");
    TGTextButton* superimposeButton       = new TGTextButton(outputControlFrameButtons, "&Superimpose");
    TGVertical3DLine* outputseperatorLine = new TGVertical3DLine(controlFrame);

    outputControlFrameButtons->AddFrame(mergeSelectionButton, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    outputControlFrameButtons->AddFrame(superimposeButton,    new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));

        // ------- Custom
    TGVerticalFrame* controlFrameCustom = new TGVerticalFrame(controlFrame, 200, 40);

    renameCheckbox = new TGCheckButton(controlFrameCustom,"Use Custom Title");
    renameTextbox  = new TGTextEntry(controlFrameCustom);

            // X Axis
    TGHorizontalFrame* controlFrameXRange = new TGHorizontalFrame(controlFrameCustom, 200, 40);

    xRangeCheckbox    = new TGCheckButton(controlFrameCustom,"Use Custom X Range");
    xminNumbertextbox = new TGNumberEntryField(controlFrameXRange);
    xmaxNumbertextbox = new TGNumberEntryField(controlFrameXRange);

    controlFrameXRange->AddFrame(xminNumbertextbox, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameXRange->AddFrame(xmaxNumbertextbox, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));

            // Y Axis
    TGHorizontalFrame* controlFrameYRange = new TGHorizontalFrame(controlFrameCustom, 200, 40);

    yRangeCheckbox    = new TGCheckButton(controlFrameCustom,"Use Custom Y Range");
    yminNumbertextbox = new TGNumberEntryField(controlFrameYRange);
    ymaxNumbertextbox = new TGNumberEntryField(controlFrameYRange);

    controlFrameYRange->AddFrame(yminNumbertextbox, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameYRange->AddFrame(ymaxNumbertextbox, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));

        // ------- Add to Custom
    controlFrameCustom->AddFrame(renameCheckbox,     new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameCustom->AddFrame(renameTextbox,      new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));

    controlFrameCustom->AddFrame(xRangeCheckbox,     new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameCustom->AddFrame(controlFrameXRange, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));

    controlFrameCustom->AddFrame(yRangeCheckbox,     new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameCustom->AddFrame(controlFrameYRange, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));

        // --- Add to All Controls
    controlFrame->AddFrame(selectionControlFrameButtons, new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));
    controlFrame->AddFrame(controlFrameCheckboxes,       new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));
    controlFrame->AddFrame(controlFrameCustom,           new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));
    controlFrame->AddFrame(outputseperatorLine,          new TGLayoutHints(kLHintsExpandY, 2, 2, 2, 2));
    controlFrame->AddFrame(outputControlFrameButtons,    new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));

    // ---- Main Frame, Adding all the individual frames in the appropriate order top to bottom
    fMain->AddFrame(fMenuBar,           new TGLayoutHints(kLHintsLeft ,2,2,2,2));
    fMain->AddFrame(quicksearchFrame,   new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX,2,2,2,2));
    fMain->AddFrame(currdirLabel,       new TGLayoutHints(kLHintsLeft | kLHintsExpandX ,2,2,2,2));
    fMain->AddFrame(listboxesFrame,     new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 6));
    fMain->AddFrame(controlFrame,       new TGLayoutHints(kLHintsLeft, 2, 2, 2, 2));

    fMain->SetWindowName("Filter & Combine Plots");
    fMain->MapSubwindows();

    // #### Signal/Slots ####

    fMenuFile->Connect("Activated(Int_t)", "MyMainFrame", this, "HandleMenu(Int_t)");

    searchBox->Connect("TextChanged(const char *)", "MyMainFrame", this, "FilterBySearchBox()");
    displayPathCheckBox->Connect("Clicked()", "MyMainFrame", this, "UpdateDistplayListboxes()");

    mainListBox->Connect("DoubleClicked(Int_t)", "MyMainFrame", this, "AddToSelection(Int_t)");
    selectionListBox->Connect("DoubleClicked(Int_t)", "MyMainFrame", this, "RemoveFromSelection(Int_t)");

    clearSelectionButton->Connect("Clicked()", "MyMainFrame", this, "ClearSelectionListbox()");
    previewSelectionButton->Connect("Clicked()", "MyMainFrame", this, "PreviewSelection()");
    superimposeButton->Connect("Clicked()", "MyMainFrame", this, "Superimpose()");
    mergeSelectionButton->Connect("Clicked()", "MyMainFrame", this, "MergeSelection()");

    renameCheckbox->Connect("Clicked()", "MyMainFrame", this, "ToggleEnableRenameTextbox()");
    xRangeCheckbox->Connect("Clicked()", "MyMainFrame", this, "ToggleXMaxTextbox()");
    yRangeCheckbox->Connect("Clicked()", "MyMainFrame", this, "ToggleYMaxTextbox()");
    tdrstyleCheckBox->Connect("Clicked()", "MyMainFrame", this, "SetStyle()");

    // #### Init Window ####

    fMain->MapWindow();
    fMain->MoveResize(100, 100, 520, 700);
    ResetGuiElements();

//    InitAll();
//    searchBox->SetText("NumberOfClusters");
}

MyMainFrame::~MyMainFrame() {
    fMain->Cleanup();
    delete fMain;
}

void MyMainFrame::HandleMenu(Int_t menu_id) {
    switch (menu_id) {
    case M_FILE_OPEN:
        cout << "[ OK ] Open File Dialog" << endl;
        InitAll();
        break;

    case M_FILE_EXIT:
        gApplication->Terminate(0);
        break;
    }
}

string MyMainFrame::LoadFileFromDialog() {
    TGFileInfo file_info_;
    const char *filetypes[] = {"ROOT files", "*.root", 0, 0};
    file_info_.fFileTypes = filetypes;
    loadDialog = new TGFileDialog(gClient->GetDefaultRoot(), fMain, kFDOpen, &file_info_);

    return file_info_.fFilename ? file_info_.fFilename : "";
}

void MyMainFrame::ToggleEnableRenameTextbox() {
    renameTextbox->SetEnabled(renameCheckbox->IsDown());
}

void MyMainFrame::ToggleXMaxTextbox() {
    xminNumbertextbox->SetEnabled(xRangeCheckbox->IsDown());
    xmaxNumbertextbox->SetEnabled(xRangeCheckbox->IsDown());
}

void MyMainFrame::ToggleYMaxTextbox() {
    yminNumbertextbox->SetEnabled(yRangeCheckbox->IsDown());
    ymaxNumbertextbox->SetEnabled(yRangeCheckbox->IsDown());
}

void MyMainFrame::ResetGuiElements() {
    currdirLabel->SetText("");
    searchBox->SetText("");
    renameTextbox->SetEnabled(false);
    xminNumbertextbox->SetEnabled(false);
    xmaxNumbertextbox->SetEnabled(false);
    yminNumbertextbox->SetEnabled(false);
    ymaxNumbertextbox->SetEnabled(false);

    displayPathCheckBox->SetState(kButtonUp);
    statsCheckBox->SetState(kButtonUp);
    legendCheckBox->SetState(kButtonUp);
    normalizeCheckBox->SetState(kButtonUp);
    tdrstyleCheckBox->SetState(kButtonUp);

    SetStyle();
}

void MyMainFrame::InitAll() {
    ResetGuiElements();

    string file_name = LoadFileFromDialog();
//    string file_name = "/home/fil/projects/GuiPlotTool/tmp/DQM_V0001_SiStrip_R000283283.root";
    file = TFile::Open(file_name.c_str());

    if(file) {
        cout << "[ OK ] Opening File" << endl;
    } else {
        cout << "[FAIL] Opening File" << endl;
        return;
    }

    string baseDirStr = "DQMData";
    baseDir = (TDirectory*)file->Get(baseDirStr.c_str());

    baseDir ?  cout << "[ OK ]" : cout << "[FAIL]";
    cout << " Entering Directory" << endl;

    string currentPath = file_name + ":/" + string(baseDirStr);
    currdirLabel->SetText(currentPath.c_str());

    // Fill internal data structes from file and display
    LoadAllPlotsFromDir(baseDir);
    DisplayMapInListBox(table, mainListBox);
}

void MyMainFrame::UpdateDistplayListboxes() {
    DisplayMapInListBox(table, mainListBox);
    DisplayMapInListBox(selection, selectionListBox);
    FilterBySearchBox();
}

void MyMainFrame::LoadAllPlotsFromDir(TDirectory *current) {
    TIter next(current->GetListOfKeys());
    TKey* key;

    while ((key = (TKey* )next())) {
        TClass* cl = gROOT->GetClass(key->GetClassName());

        if (cl->InheritsFrom("TDirectory")) {
            TDirectory* d = (TDirectory*)key->ReadObj();
            LoadAllPlotsFromDir(d);
        }

        if (cl->InheritsFrom("TH1")) {
            TH1* h = (TH1* )key->ReadObj();
            string fname = h->GetName();
            string path = string(current->GetPath()) + "/" + fname;

            Int_t key_entry = GetNextFreeId();
            HistogramInfo value_entry(h, path);

            table.insert(make_pair(key_entry, value_entry));
        }
    }
}

void MyMainFrame::DisplayMapInListBox(const map<Int_t, HistogramInfo>& m, TGListBox* listbox) {
    listbox->RemoveAll();

    string disp;
    for (auto& elem : m) {
        displayPathCheckBox->IsDown() ? disp = elem.second.GetPath() : disp = elem.second.GetName();
        listbox->AddEntry(disp.c_str(), elem.first);
    }
    listbox->SortByName();
}

void MyMainFrame::AddToSelection(Int_t id) {
    HistogramInfo val = table.find(id)->second;
    selection.insert(make_pair(id, val));
    DisplayMapInListBox(selection, selectionListBox);
}

void MyMainFrame::RemoveFromSelection(Int_t id) {
    selection.erase(id);
    DisplayMapInListBox(selection, selectionListBox);
}

void MyMainFrame::ClearSelectionListbox() {
    selection.clear();
    DisplayMapInListBox(selection, selectionListBox);
}

void MyMainFrame::FilterBySearchBox() {
    mainListBox->RemoveAll();

    map<Int_t, HistogramInfo> tmp_filtered;

    string seach_text = searchBox->GetText();

    for (auto &elem : table) {
        string fullname = elem.second.GetPath();
        if (fullname.find(seach_text) != string::npos) {
            tmp_filtered.insert(elem);
        }
    }
    DisplayMapInListBox(tmp_filtered, mainListBox);
}

void MyMainFrame::PreviewSelection() {
    if(resultCanvas) delete resultCanvas;

    resultCanvas = new TCanvas("Preview Canvas", "", 800, 400);
    resultCanvas->Divide(selection.size(), 1);

    // never work on the originals - they are still on disk (file)
    vector<TH1*> copies;
    copies.clear();

    for(auto& elem : selection){
        copies.push_back((TH1*)elem.second.GetObj()->Clone());
    }

    Int_t i = 1;
    for (auto& elem : copies) {
        resultCanvas->cd(i++);

        if(normalizeCheckBox->IsOn()) {
            elem->DrawNormalized();
        } else {
            elem->Draw();
        }
    }
}

// statboxes : OUT param
void MyMainFrame::CreateStatBoxes(vector<TH1*>& plots, vector<TPaveStats*>& statboxes) {
    TPaveStats* tstat;
    double X1, Y1, X2, Y2;

    TCanvas* tmpCanvas = new TCanvas("a Canvas", "", 800, 400);
    tmpCanvas->cd();

    for(int i=0; i<plots.size(); i++) {

        plots[i]->Draw();
        gPad->Update();

        tstat = (TPaveStats*) plots[i]->FindObject("stats");

        if(i!=0){
            tstat->SetX1NDC(X1);
            tstat->SetX2NDC(X2);
            tstat->SetY1NDC(Y1-(Y2-Y1));
            tstat->SetY2NDC(Y1);
        }

        X1 = tstat->GetX1NDC();
        Y1 = tstat->GetY1NDC();
        X2 = tstat->GetX2NDC();
        Y2 = tstat->GetY2NDC();

        statboxes.push_back(tstat);
    }
    delete tmpCanvas;

}

void MyMainFrame::SetCheckboxOptions(TH1* elem) {
    if(xRangeCheckbox->IsOn()) {
        elem->SetAxisRange(xminNumbertextbox->GetNumber(), xmaxNumbertextbox->GetNumber(),"X");
    }

    if(yRangeCheckbox->IsOn()) {
        elem->SetAxisRange(yminNumbertextbox->GetNumber(), ymaxNumbertextbox->GetNumber(),"Y");
    }    

    if(renameCheckbox->IsOn()) {
        string tmp = renameTextbox->GetText();
        elem->SetTitle(tmp.c_str());
    }

    elem->SetStats(statsCheckBox->IsOn());
}

void MyMainFrame::DrawPlots(vector<TH1*>& plots, vector<TPaveStats*>& statboxes, string option="") {

    vector<Int_t> basic_colors = { kBlue, kGreen, kCyan, kMagenta, kRed };
    vector<Int_t> colors;

    // why god why... https://root.cern.ch/doc/master/classTColor.html
    // Adding non +1 increments of basecolors to achieve maximum difference
    // between the first few colors that are being used for the plots
    for(auto c : basic_colors) colors.push_back(c);
    for(auto c : basic_colors) colors.push_back(c+2);
    for(auto c : basic_colors) colors.push_back(c-7);
    for(auto c : basic_colors) colors.push_back(c-4);
    for(auto c : basic_colors) colors.push_back(c-9);

    auto legend = new TLegend(0.1,0.7,0.48,0.9);

    Int_t idx = 0;
    for(auto& elem : plots) {
        elem->SetLineColor(colors[idx]);
        legend->AddEntry(elem, elem->GetTitle());
        SetCheckboxOptions(elem);

        if(normalizeCheckBox->IsOn()) {
            elem->DrawNormalized(option.c_str());
        } else {
            elem->Draw(option.c_str());
        }

        idx++;
    }

    if(statsCheckBox->IsOn()) {
        idx = 0;
        for(auto& elem : statboxes) {
            elem->SetTextColor(colors[idx]);
            elem->SetLineColor(colors[idx]);
            elem->Draw(option.c_str());

            idx++;
        }
    }

    if(legendCheckBox->IsOn()) {
        legend->Draw();
    }
}

void MyMainFrame::Superimpose() {
    if(resultCanvas) delete resultCanvas;

    resultCanvas = new TCanvas("Result", "", 800, 400);
    resultCanvas->cd();

    // never work on the originals - they are still on disk (file)
    vector<TH1*> copies;
    copies.clear();

    for(auto& elem : selection){
        copies.push_back((TH1*)elem.second.GetObj()->Clone());
    }

    vector<TPaveStats*> statboxes; // out param
    statboxes.clear();

    CreateStatBoxes(copies, statboxes);
    DrawPlots(copies, statboxes, "same");

}

void MyMainFrame::MergeSelection() {
    if(resultCanvas) delete resultCanvas;

    resultCanvas = new TCanvas("Result", "", 800, 400);
    resultCanvas->cd();

    // never work on the originals - they are still on disk (file)
    vector<TH1*> copies;
    copies.clear();

    for(auto& elem : selection){
        copies.push_back((TH1*)elem.second.GetObj()->Clone());
    }

    auto legend = new TLegend(0.1,0.7,0.48,0.9);

    if(copies.size() != 0) {
        for(Int_t idx=1; idx<copies.size(); ++idx) {
            copies[0]->Add(copies[idx]);
        }

        SetCheckboxOptions(copies[0]);
        copies[0]->Draw();
    }

    if(legendCheckBox->IsOn()) {
        legend->Draw();
    }
}

void MyMainFrame::SetStyle() {
    if(current_style != nullptr) {
        delete current_style;
        current_style = nullptr;
    }
    InitStyles();
}

void MyMainFrame::InitStyles() {
    if(current_style == nullptr) {
        if(tdrstyleCheckBox->IsOn()){
            current_style = new TStyle("tdrStyle","Style for P-TDR");
            SetPublicationStyle(current_style);
        } else {
            current_style = new TStyle("Modern","");
        }
    }
    current_style->cd();
}

void MyMainFrame::SetPublicationStyle(TStyle* style) {
    style->SetCanvasBorderMode(0);
    style->SetCanvasColor(kWhite);
    style->SetCanvasDefX(0);
    style->SetCanvasDefY(0);
    style->SetPadBorderMode(0);
    style->SetPadColor(kWhite);
    style->SetPadGridX(false);
    style->SetPadGridY(false);
    style->SetGridColor(0);
    style->SetGridStyle(3);
    style->SetGridWidth(1);
    style->SetFrameBorderMode(0);
    style->SetFrameBorderSize(1);
    style->SetFrameFillColor(0);
    style->SetFrameFillStyle(0);
    style->SetFrameLineColor(1);
    style->SetFrameLineStyle(1);
    style->SetFrameLineWidth(1);
    style->SetHistLineColor(1);
    style->SetHistLineStyle(0);
    style->SetHistLineWidth(1);
    style->SetErrorX(0.);
    style->SetMarkerStyle(21);
    style->SetMarkerSize(.5);
    style->SetOptFit(1);
    style->SetFitFormat("5.4g");
    style->SetFuncColor(2);
    style->SetFuncStyle(1);
    style->SetFuncWidth(1);
    style->SetOptDate(0);
    style->SetOptFile(0);
    style->SetOptStat("mr");
    style->SetStatColor(kWhite);
    style->SetStatFont(42);
    style->SetStatFontSize(0.025);
    style->SetStatTextColor(1);
    style->SetStatFormat("6.4g");
    style->SetStatBorderSize(1);
    style->SetStatH(0.1);
    style->SetStatW(0.15);
    style->SetPadTopMargin(0.05);
    style->SetPadBottomMargin(0.13);
    style->SetPadLeftMargin(0.13);
    style->SetPadRightMargin(0.05);
    style->SetOptTitle(1);
    style->SetTitleFont(42);
    style->SetTitleColor(1);
    style->SetTitleTextColor(1);
    style->SetTitleFillColor(10);
    style->SetTitleFontSize(0.045);
    style->SetTitleColor(1, "XYZ");
    style->SetTitleFont(42, "XYZ");
    style->SetTitleSize(0.06, "XYZ");
    style->SetTitleXOffset(0.9);
    style->SetTitleYOffset(1.05);
    style->SetLabelColor(1, "XYZ");
    style->SetLabelFont(42, "XYZ");
    style->SetLabelOffset(0.007, "XYZ");
    style->SetLabelSize(0.05, "XYZ");
    style->SetAxisColor(1, "XYZ");
    style->SetStripDecimals(kTRUE);
    style->SetTickLength(0.03, "XYZ");
    style->SetNdivisions(510, "XYZ");
    style->SetPadTickX(1);
    style->SetPadTickY(1);
    style->SetOptLogx(0);
    style->SetOptLogy(0);
    style->SetOptLogz(0);
}


void GuiPlotTool() {
    new MyMainFrame(gClient->GetRoot(), 200, 200);
}
