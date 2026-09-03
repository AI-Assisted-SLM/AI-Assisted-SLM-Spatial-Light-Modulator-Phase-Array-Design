#pragma once

#include "afxdialogex.h"
#include <afxcmn.h> // 确保常用 MFC 控件类型可用
#include "SlmPhaseLabDlg.h"

// PICFIND 对话框
class PICFIND : public CDialogEx
{
    DECLARE_DYNAMIC(PICFIND)

public:
    explicit PICFIND(CWnd* pParent = nullptr);   // 标准构造函数
    virtual ~PICFIND() override;

    // 禁用拷贝/赋值
    PICFIND(const PICFIND&) = delete;
    PICFIND& operator=(const PICFIND&) = delete;

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_PICFIND };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV 支持

    DECLARE_MESSAGE_MAP()

protected:
    CMFCShellTreeCtrl m_shellTree;
    CMFCShellListCtrl m_shellList;
    

public:
    afx_msg void OnTvnSelchangedMfcshelltree1(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnLvnItemchangedMfcshelllist1(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnBnClickedButtonYesFind();

    //分窗口的地址存储的变量
    CString m_selectedPath;
};
