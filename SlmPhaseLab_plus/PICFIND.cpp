// PICFIND.cpp: 实现文件
//

#include "pch.h"
#include "SlmPhaseLab.h"
#include "afxdialogex.h"
#include "PICFIND.h"


// PICFIND 对话框

IMPLEMENT_DYNAMIC(PICFIND, CDialogEx)

PICFIND::PICFIND(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PICFIND, pParent)
{

}

PICFIND::~PICFIND()
{
}

void PICFIND::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MFCSHELLTREE1, m_shellTree);
	DDX_Control(pDX, IDC_MFCSHELLLIST1, m_shellList);
}


BEGIN_MESSAGE_MAP(PICFIND, CDialogEx)
	ON_NOTIFY(TVN_SELCHANGED, IDC_MFCSHELLTREE1, &PICFIND::OnTvnSelchangedMfcshelltree1)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_MFCSHELLLIST1, &PICFIND::OnLvnItemchangedMfcshelllist1)
	ON_BN_CLICKED(IDC_BUTTON_YES_FIND, &PICFIND::OnBnClickedButtonYesFind)
END_MESSAGE_MAP()


// PICFIND 消息处理程序

void PICFIND::OnTvnSelchangedMfcshelltree1(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码

	//选择节点
	CString str_path;
	m_shellTree.GetItemPath(str_path, pNMTreeView->itemNew.hItem);
	//List展开
	m_shellList.DisplayFolder(str_path);

	*pResult = 0;
}

void PICFIND::OnLvnItemchangedMfcshelllist1(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
}

void PICFIND::OnBnClickedButtonYesFind()
{
	// TODO: 在此添加控件通知处理程序代码
	int nIndex = m_shellList.GetNextItem(-1, LVNI_SELECTED);

	if (nIndex == -1)
	{
		AfxMessageBox(_T("请先选择文件"));
		return;
	}

	CString path;

	if (m_shellList.GetItemPath(path, nIndex))
	{
		m_selectedPath = path;
	}

	EndDialog(IDOK);
}
