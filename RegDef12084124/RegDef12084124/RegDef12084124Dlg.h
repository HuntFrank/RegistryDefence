
// RegDef12084124Dlg.h : ͷ�ļ�
//

#pragma once

#define DEVICE_NAME "\\Device\\SSDTHOOKRegDevice"	// Driver Name
#define Win32LinkName "\\\\.\\SSDTHookReg"



//////////////////////////////////////////////////////////////////////////
//  ����IOCTL������
//////////////////////////////////////////////////////////////////////////
// �����¼�
#define IOCTL_SETEVENT \
	(ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x831, METHOD_BUFFERED, FILE_ANY_ACCESS)

// ��ring0 ��������
#define IOCTL_GETINFO \
	(ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x832, METHOD_BUFFERED, FILE_ANY_ACCESS)

// ���õȴ��û��жϵ��¼�
#define IOCTL_SETWAITEVENT \
	(ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x833, METHOD_BUFFERED, FILE_ANY_ACCESS)

// �����û����жϽ��
#define IOCTL_PASSUSERRES \
	(ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x834, METHOD_BUFFERED, FILE_ANY_ACCESS)


// CRegDef12084124Dlg �Ի���
class CRegDef12084124Dlg : public CDialogEx
{
// ����
public:
	CRegDef12084124Dlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_REGDEF12084124_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedStart();
	afx_msg void OnBnClickedStop();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedSavelog();
	afx_msg void OnClose();

	static UINT __cdecl ThreadProc(LPVOID pParam);
	static UINT __cdecl EventThread(LPVOID pParam);

	BOOL m_bNotLeave;
	char m_OutBuffer[1024];

	HANDLE m_hDevice;
	HANDLE m_hEvent;
	DWORD m_returnBytes;
	HANDLE m_hWaitUserRequestEvent;
	
	// 1��ʾ�����޸� 0��ʾ�������޸�
	char UserRes;
};

