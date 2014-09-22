/*
 * CExcel.cpp
 *
 *  Created on: 2013-2-20
 *      Author: allengao
 */

#include "CExcel.h"

static char const
        g_szExcelHeader[] =
                "<?xml version=\"1.0\" encoding=\"gbk\" ?>\r\n"
                    "<?mso-application progid=\"Excel.Sheet\"?>\r\n"
                    "<Workbook xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\"\r\n"
                    "xmlns:o=\"urn:schemas-microsoft-com:office:office\"\r\n"
                    "xmlns:x=\"urn:schemas-microsoft-com:office:excel\"\r\n"
                    "xmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\"\r\n"
                    "xmlns:html=\"http://www.w3.org/TR/REC-html40\">\r\n"
                    " <Styles>\r\n"
                    "<Style ss:ID=\"Default\" ss:Name=\"Normal\">"
                    "<Alignment ss:Vertical=\"Center\" ss:Horizontal=\"Center\" />"
                    "<Borders/>"
                    "<Font ss:FontName=\"ËÎÌå\" x:CharSet=\"134\" ss:Size=\"11\" ss:Color=\"#000000\"/>"
                    "<Interior/>"
                    "<NumberFormat/>"
                    "<Protection/>"
                    "</Style>"
                    " </Styles>\r\n";

CExcel::CExcel()
{
    // TODO Auto-generated constructor stub

}

CExcel::~CExcel()
{
    // TODO Auto-generated destructor stub
}

void CExcel::strToCellData(string &sResult, const ExcelCellData &ecd)
{
    stringstream oss;
    oss << "\t\t\t<Cell";
    if (ecd.nIndex > 1)
        oss << " ss:Index=\"" << ecd.nIndex << "\"";
    if (ecd.nMergeAcross > 0)
        oss << " ss:MergeAcross=\"" << ecd.nMergeAcross << "\"";
    if (ecd.nMergeDown > 0)
        oss << " ss:MergeDown=\"" << ecd.nMergeDown << "\"";
    oss << " ss:StyleID=\"Default\">";
    if ("Number" == ecd.sType)
        oss << "<Data ss:Type=\"Number\">";
    else
        oss << "<Data ss:Type=\"String\">";
    oss << ecd.sContent << "</Data></Cell>\r\n";
    sResult = oss.str();
}

void CExcel::vectToRowData(string &sResult,
        const vector<ExcelCellData> & vectECD)
{
    sResult = "\t\t<Row>\r\n";
    for(vector<ExcelCellData>::const_iterator vectIter = vectECD.begin(); vectIter
            != vectECD.end(); ++vectIter)
    {
        string tmp = "";
        strToCellData(tmp, *vectIter);
        sResult += tmp;
    }
    sResult += "\t\t</Row>\r\n";
}

void CExcel::vectToWorkSheet(string &sResult, const string &sWorkSheetName,
        const vector<vector<ExcelCellData> > &vectData)
{
    sResult
            = "\t <Worksheet ss:Name=\"" + sWorkSheetName
                    + "\">\r\n\t\t<Table ss:DefaultColumnWidth=\"80\" ss:DefaultRowHeight=\"20\">\r\n";
    for(vector<vector<ExcelCellData> >::const_iterator vectIter =
            vectData.begin(); vectIter != vectData.end(); ++vectIter)
    {
        string tmp = "";
        vectToRowData(tmp, *vectIter);
        sResult += tmp;
    }
    sResult += "\t\t</Table>\r\n\t</Worksheet>\r\n";
}

void CExcel::outputExcelFile(const string &sDir, const string &sFile,
        string &sContent)
{
    ofstream fout((sDir + "/" + sFile).c_str(), ifstream::out);
    if (!fout.good())
    {
        cout << "open file error: " << sDir + "/" + sFile << endl;
        return;
    }

    sContent = g_szExcelHeader + sContent + "</Workbook>";
    //fout << "Content-Type: application/vnd.ms-excel\r\n";
    //fout << "Content-Disposition:attachment; filename=" << sFile << "\r\n";
    //fout << "Content-Length: " << sContent.length();
    //fout << "\r\n\r\n";
    fout << sContent << endl;

    fout.close();
}

string
        g_sHtmlHead =
                "<html><head>"
                    "<meta http-equiv=\"Content-Type\" content=\"text/html; "
                    "charset=gbk\"></head>"
                    "<body><table width=\"100%\" border=\"1\" cellpadding=\"1\" cellspacing=\"0\" "
                    "align=\"center\" style=\"font-family:Î¢ÈíÑÅºÚ;font-size:15px\">";

void strToCellHtml(string &sResult, const ExcelCellData &ecd)
{
    if ("" == ecd.sContent)
        return;

    stringstream oss;
    oss << "<td";
    if (ecd.nMergeAcross > 0)
        oss << " colspan=\"" << ecd.nMergeAcross + 1 << "\"";
    if (ecd.nMergeDown > 0)
        oss << " rowspan=\"" << ecd.nMergeDown + 1 << "\"";
    oss << ">";
    oss << ecd.sContent << "</td>";
    sResult = oss.str();
}

void vectToRowHtml(string &sResult, const vector<ExcelCellData> & vectECD)
{
    sResult = "<tr>";
    for(vector<ExcelCellData>::const_iterator vectIter = vectECD.begin(); vectIter
            != vectECD.end(); ++vectIter)
    {
        string tmp = "";
        strToCellHtml(tmp, *vectIter);
        sResult += tmp;
    }
    sResult += "</tr>";
}

void CExcel::outputHtml(string &sHtml,
        const vector<vector<ExcelCellData> > &vectData)
{
    sHtml = g_sHtmlHead;
    for(vector<vector<ExcelCellData> >::const_iterator vectIter =
            vectData.begin(); vectIter != vectData.end(); ++vectIter)
    {
        string tmp = "";
        vectToRowHtml(tmp, *vectIter);
        sHtml += tmp;
    }

    sHtml += "</table></body></html>";
}
