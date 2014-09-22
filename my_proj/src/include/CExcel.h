/*
 * CExcel.h
 *
 *  Created on: 2013-2-20
 *      Author: allengao
 */

#ifndef CEXCEL_H_
#define CEXCEL_H_

#include <string>
#include <map>
#include <vector>
#include <set>
#include <iterator>
#include <sstream>
#include <iostream>
#include <fstream>

using namespace std;

struct ExcelCellData
{
    int nIndex;
    int nMergeAcross;
    int nMergeDown;
    string sContent;
    string sType;
    ExcelCellData() :
        nIndex(1), nMergeAcross(0), nMergeDown(0)
    {
        sType = "String";
        sContent = "";
    }
    ExcelCellData(const string &sContent) :
        nIndex(1), nMergeAcross(0), nMergeDown(0), sContent(sContent)
    {
        sType = "String";
    }
    ExcelCellData(const int nIndex, const int nAcross, const int nDown,
            const string &sContent) :
        nIndex(nIndex), nMergeAcross(nAcross), nMergeDown(nDown), sContent(
                sContent)
    {
        sType = "String";
    }

    ExcelCellData(const string &sContent, const string &sType) :
        nIndex(1), nMergeAcross(0), nMergeDown(0), sContent(sContent), sType(
                sType)
    {
    }

};

class CExcel
{
public:
    CExcel();
    virtual ~CExcel();

public:
    void strToCellData(string &sResult, const ExcelCellData &ecd);
    void vectToRowData(string &sResult, const vector<ExcelCellData> & vectECD);
    void vectToWorkSheet(string &sResult, const string &sWorkSheetName,
            const vector<vector<ExcelCellData> > &vectData);
    void outputExcelFile(const string &sDir, const string &sFile,
            string &sContent);

    void outputHtml(string &sHtml,
            const vector<vector<ExcelCellData> > &vectData);
};

#endif /* CEXCEL_H_ */
