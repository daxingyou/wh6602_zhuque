#include "StdAfx.h"
#include "AttemperEngineSink.h"
#include "Zip.h"
#include "LogonServer.h"
#include "TraceCatch.h"

#define MANY_ENTER	0 // 多开

//声明变量
extern CLogonServerApp theApp;
//////////////////////////////////////////////////////////////////////////

//宏定义
#define TIME_RECONNECT						15000L						//重连时间
#define TIME_UPDATE_LIST					30000L						//更新时间
#define TIME_DOWNLOAD_CUSTOM_FACE			200L						//下载头像
#define TIME_READ_GRADE_INFO                500L                        //读取等级信息
#define TIME_CONTINUE_CONNECT				(1000*600)					//连接数据库时间						Author:<cxf>

//定时器 ID
#define IDI_CONNECT_CENTER_SERVER			1							//连接服务器
#define IDI_UPDATE_SERVER_LIST				2							//更新列表
#define IDI_DOWNLOAD_CUSTOM_FACE			3							//下载头像

#define IDI_READ_GRADE_INFO                 4                           //读取等级信息
#define IDI_CONTINUE_CONNECT				5							//连接数据库定时器ID					Author:<cxf>

//////////////////////////////////////////////////////////////////////////

//构造函数
CAttemperEngineSink::CAttemperEngineSink()
{
	//设置变量
	//m_pInitParamter=NULL;
	m_pInitParamter1=NULL;
	m_pBindParameter=NULL;

	//设置变量
	m_pITimerEngine=NULL;
	m_pIDataBaseEngine=NULL;
	m_pITCPNetworkEngine=NULL;
	m_pITCPSocketCorrespond=NULL;

	m_pGradeInfo = 0;
	m_iGradeNum  = 0;
	return;
}

//析构函数
CAttemperEngineSink::~CAttemperEngineSink()
{
	delete m_pGradeInfo;
	m_pGradeInfo = 0;

	MMachine::iterator M_const_return = m_hmMachineCode.begin();
	for (; m_hmMachineCode.end() != M_const_return; M_const_return++)
	{
		MachineData * pData = M_const_return->second;
		delete pData;
	}
	m_hmMachineCode.clear();
}

//接口查询
void * __cdecl CAttemperEngineSink::QueryInterface(const IID & Guid, DWORD dwQueryVer)
{
	QUERYINTERFACE(IAttemperEngineSink,Guid,dwQueryVer);
	QUERYINTERFACE_IUNKNOWNEX(IAttemperEngineSink,Guid,dwQueryVer);
	return NULL;
}

//调度模块启动
bool __cdecl CAttemperEngineSink::OnAttemperEngineStart(IUnknownEx * pIUnknownEx)
{
	__ENTER_FUNCTION
	//设置组件
	m_ServerList.SetSocketEngine(m_pITCPNetworkEngine);

	//绑定参数
	m_pBindParameter=new tagBindParameter[m_pInitParamter1->m_wMaxConnect];
	ZeroMemory(m_pBindParameter,sizeof(tagBindParameter)*m_pInitParamter1->m_wMaxConnect);

	//连接中心服
	DWORD dwServerIP=inet_addr(m_pInitParamter1->m_CorrespondAddress.szAddress);
	//dwServerIP 值是反的
	m_pITCPSocketCorrespond->Connect(ntohl(dwServerIP),m_pInitParamter1->m_wCorrespondPort);
	//不能用下面这句
	//m_pITCPSocketCorrespond->Connect(m_pInitParamter->m_szCenterServerAddr,PORT_CENTER_SERVER);

	//初始数据
	//m_DownloadFaceInfoMap.InitHashTable(503);
	//m_DownloadFaceInfoMap.RemoveAll();

	//设置定时器
	//m_pITimerEngine->SetTimer(IDI_DOWNLOAD_CUSTOM_FACE,TIME_DOWNLOAD_CUSTOM_FACE,TIMES_INFINITY,NULL);


	m_pITimerEngine->SetTimer(IDI_READ_GRADE_INFO, TIME_READ_GRADE_INFO, 1,NULL);

	m_pITimerEngine->SetTimer(IDI_CONTINUE_CONNECT, TIME_CONTINUE_CONNECT, TIMES_INFINITY, NULL);//Author:<cxf>

	__LEAVE_FUNCTION
	return true;
}

//调度模块关闭
bool __cdecl CAttemperEngineSink::OnAttemperEngineStop(IUnknownEx * pIUnknownEx)
{
	//设置变量
	m_pITimerEngine=NULL;
	m_pIDataBaseEngine=NULL;
	m_pITCPNetworkEngine=NULL;
	m_pITCPSocketCorrespond=NULL;

	//删除数据
	SafeDeleteArray(m_pBindParameter);
	m_hmMachineCode.clear();
	return true;
}

//定时器事件
bool __cdecl CAttemperEngineSink::OnEventTimer(DWORD dwTimerID, WPARAM wBindParam)
{
	__ENTER_FUNCTION
	switch (dwTimerID)
	{
	case IDI_CONNECT_CENTER_SERVER:		//连接中心服务器
		{
			//连接中心服
			DWORD dwServerIP=inet_addr(m_pInitParamter1->m_CorrespondAddress.szAddress);
			//dwServerIP 值是反的
			m_pITCPSocketCorrespond->Connect(ntohl(dwServerIP),m_pInitParamter1->m_wCorrespondPort);
			//不能用下面这句
			//m_pITCPSocketCorrespond->Connect(m_pInitParamter->m_szCenterServerAddr,PORT_CENTER_SERVER);

			//错误提示
			CTraceService::TraceString(TEXT("正在尝试重新连接中心服务器...."),TraceLevel_Normal);
			return true;
		}
	case IDI_UPDATE_SERVER_LIST:		//更新服务器列表
		{
			m_pITCPSocketCorrespond->SendData(MDM_CS_SERVER_LIST,SUB_CS_GET_SERVER_LIST,NULL,0);

			return true;
		}

	case IDI_READ_GRADE_INFO: //读取等级信息
		{
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_READ_GRADE_INFO, 0, 0, 0);
			return true;
		}
	case IDI_CONTINUE_CONNECT://常连接数据库(保持数据库连接)
		{
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_CONTINUE_CONNECT, 0, 0, 0);			//Author:<cxf>
		}
	}
	__LEAVE_FUNCTION
	return false;
}

//数据库事件
bool __cdecl CAttemperEngineSink::OnEventDataBase(WORD wRequestID, DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	switch (wRequestID)
	{
	case DBR_GP_LOGON_ERROR:			//登录失败
		{
			return OnDBLogonError(dwContextID,pData,wDataSize);
		}
	case DBR_GP_LOGON_SUCCESS:			//登录成功
		{
			return OnDBLogonSuccess(dwContextID,pData,wDataSize);
		}
	case DBR_GP_UPDATE_USER_SUCC:       //更新用户信息成功
		{
			return OnUpdateUserInfoSucc(dwContextID,pData,wDataSize);
		}
	case DBR_GP_UPDATE_USER_FAIL: //更新失败
		{
			return OnUpdateUserInfoFail(dwContextID,pData,wDataSize);
		}
	case DBR_GP_DOWNLOADFACE_SUCCESS:	//下载成功
		{
			return OnDBDownloadFaceSuccess(dwContextID,pData,wDataSize);
		}
	case DBR_GP_UPLOAD_FACE_RESULT:		//上传结果
		{
			return OnDBRCustomFaceUploadResult(dwContextID,pData,wDataSize);
		}
	case DBR_GP_DELETE_FACE_RESULT:		//删除结果
		{
			return OnDBRCustomFaceDeleteResult(dwContextID,pData,wDataSize);
		}
	case DBR_GP_MODIFY_RESULT:			//修改结果
		{
			return OnDBRModifyIndividual(dwContextID,pData,wDataSize);
		}
	case DBR_GR_GRADE_INFO_NUM: //等级的数目
		{
			return OnDBRGradeInfoNum(dwContextID, pData, wDataSize);
		}
	case DBR_GR_GRADE_INFO_ONE: //得到等级数目
		{
			return OnDBRGradeInfoOne(dwContextID, pData, wDataSize);
		}

	 case DBR_GP_GIFT_MONEY_RESULT: //每天一次的免费领取
		 {
			 return OnDBRGiftMoneyResult(dwContextID, pData, wDataSize);
		 }
	}

	return false;
}

//用户登录成功
bool CAttemperEngineSink::OnDBLogonSuccess(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	__ENTER_FUNCTION
	/*char info[BUFSIZ] = {0};
	sprintf(info, _T("OnDBLogonSuccess"));
	CTraceService::TraceString(info, TraceLevel_Normal);*/

	//效验参数
	ASSERT(wDataSize==sizeof(DBR_GP_LogonSuccess));
	if (wDataSize!=sizeof(DBR_GP_LogonSuccess)) return false;

	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParamter1->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	BYTE cbBuffer[SOCKET_PACKET] = {0};
	DBR_GP_LogonSuccess * pDBRLogonSuccess=(DBR_GP_LogonSuccess *)pData;
	CMD_GP_LogonSuccess * pCMDLogonSuccess=(CMD_GP_LogonSuccess *)cbBuffer;

#if MANY_ENTER	// 暂时去掉多开， // del by HouGuoJiang 2012-03-27
	if (ANDROID != pDBRLogonSuccess->utIsAndroid)
	{
		MMachine::iterator M_const_return = m_hmMachineCode.find(pDBRLogonSuccess->szComputerID);
		if (m_hmMachineCode.end() == M_const_return)
		{
			tagMachineCode * pMachinecode = new tagMachineCode;
			pMachinecode->SetVal(pDBRLogonSuccess->szAccounts);
			m_hmMachineCode.insert(std::pair<std::string, MachineData *>(pDBRLogonSuccess->szComputerID, pMachinecode));
		}
		else
		{
			// 同一台机器3小时内，禁止登录两个不同的账户
			if (M_const_return->second->compare(pDBRLogonSuccess->szAccounts, 3600*3))
			{
				DBR_GP_LogonError LogonError;
				ZeroMemory(&LogonError,sizeof(LogonError));
				LogonError.lErrorCode=-1; 
				lstrcpyn(LogonError.szErrorDescribe,TEXT("抱歉！一台机器只允许登录一个客户端。"),CountArray(LogonError.szErrorDescribe));
				OnDBLogonError(dwContextID, &LogonError, sizeof(LogonError));
				return true;
			}
		}
	}
#endif


	//构造数据
	pCMDLogonSuccess->wFaceID=pDBRLogonSuccess->wFaceID;
	pCMDLogonSuccess->cbGender=pDBRLogonSuccess->cbGender;
	pCMDLogonSuccess->cbMember=pDBRLogonSuccess->cbMember;
	pCMDLogonSuccess->dwUserID=pDBRLogonSuccess->dwUserID;
	pCMDLogonSuccess->dwGameID=pDBRLogonSuccess->dwGameID;
	pCMDLogonSuccess->dwExperience=pDBRLogonSuccess->dwExperience;
	pCMDLogonSuccess->dwCustomFaceVer=pDBRLogonSuccess->dwCustomFaceVer;
	pCMDLogonSuccess->dwLockServerID =pDBRLogonSuccess->dwLockServerID;

	//彭文添加
	lstrcpyn(pCMDLogonSuccess->szNickName, pDBRLogonSuccess->szNickName, CountArray(pCMDLogonSuccess->szNickName));	
	lstrcpyn(pCMDLogonSuccess->szKey, pDBRLogonSuccess->szKey, CountArray(pCMDLogonSuccess->szKey));		
	pCMDLogonSuccess->lMoney       = pDBRLogonSuccess->lMoney ; 											
	pCMDLogonSuccess->lGold        = pDBRLogonSuccess->lGold;                              
	pCMDLogonSuccess->lGem         = pDBRLogonSuccess->lGem;                             
	pCMDLogonSuccess->dwGrade      = pDBRLogonSuccess->dwGrade; 

	pCMDLogonSuccess->lWinCount    = pDBRLogonSuccess->lWinCount;							//胜利盘数
	pCMDLogonSuccess->lLostCount   = pDBRLogonSuccess->lLostCount;							//失败盘数
	pCMDLogonSuccess->lDrawCount   = pDBRLogonSuccess->lDrawCount;							//和局盘数
	pCMDLogonSuccess->lFleeCount   = pDBRLogonSuccess->lFleeCount;							//断线数目
	pCMDLogonSuccess->utIsAndroid  = pDBRLogonSuccess->utIsAndroid;							//是否是机器人
	pCMDLogonSuccess->lGiftNum     = pDBRLogonSuccess->lGiftNum; 
	pCMDLogonSuccess->nMasterRight = pDBRLogonSuccess->nMasterRight;						//权限管理
	pCMDLogonSuccess->nMasterOrder = pDBRLogonSuccess->nMasterOrder;						//权限等级
	//添加结束

	// add by HouGuoJiang 2011-11-25
	_snprintf(pCMDLogonSuccess->szHashID,  sizeof(pCMDLogonSuccess->szHashID), "%s", pDBRLogonSuccess->szHashID);

	//叠加数据
	CSendPacketHelper SendPacket(cbBuffer+sizeof(CMD_GP_LogonSuccess),sizeof(cbBuffer)-sizeof(CMD_GP_LogonSuccess));
	SendPacket.AddPacket(pDBRLogonSuccess->szPassword,CountStringBuffer(pDBRLogonSuccess->szPassword),DTP_USER_PASS);
	SendPacket.AddPacket(pDBRLogonSuccess->szAccounts,CountStringBuffer(pDBRLogonSuccess->szAccounts),DTP_USER_ACCOUNTS);
	SendPacket.AddPacket(pDBRLogonSuccess->szUnderWrite,CountStringBuffer(pDBRLogonSuccess->szUnderWrite),DTP_UNDER_WRITE);

	//社团信息
	if (pDBRLogonSuccess->szGroupName[0]!=0)
	{
		SendPacket.AddPacket(pDBRLogonSuccess->szGroupName,CountStringBuffer(pDBRLogonSuccess->szGroupName),DTP_USER_GROUP_NAME);
	}

	//站点主页
	if (m_pInitParamter1->m_ServiceAddress.szAddress[0]!=0)
	{
		SendPacket.AddPacket(m_pInitParamter1->m_ServiceAddress.szAddress,CountStringBuffer(m_pInitParamter1->m_ServiceAddress.szAddress),DTP_STATION_PAGE);
	}

	//发送登录结果
	WORD wSendSize=sizeof(CMD_GP_LogonSuccess)+SendPacket.GetDataSize();
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_LOGON,SUB_GP_LOGON_SUCCESS,cbBuffer,wSendSize);

	/*sprintf(info, _T("OnDBLogonSuccess SUB_GP_LOGON_SUCCESS"));
	CTraceService::TraceString(info, TraceLevel_Normal);*/

	//列表配置
	CMD_GP_ListConfig ListConfig;
	ListConfig.bShowOnLineCount=TRUE;
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_SERVER_LIST,SUB_GP_LIST_CONFIG,&ListConfig,sizeof(ListConfig));

	/*sprintf(info, _T("OnDBLogonSuccess SUB_GP_LIST_CONFIG"));
	CTraceService::TraceString(info, TraceLevel_Normal);*/
	//发送游戏列表
	m_ServerList.SendGameTypeList(dwContextID);
	m_ServerList.SendGameKindList(dwContextID);
	m_ServerList.SendGameStationList(dwContextID);
	m_ServerList.SendGameServerList(dwContextID);

	m_pITCPNetworkEngine->SendData(dwContextID,  MDM_GP_LOGON, SUB_GP_LOGON_USER_GRADE_INFO, //发送等级信息
		m_pGradeInfo, sizeof(CMD_GP_UserGradeInfo) * m_iGradeNum);

	/*sprintf(info, _T("OnDBLogonSuccess SUB_GP_LOGON_USER_GRADE_INFO"));
	CTraceService::TraceString(info, TraceLevel_Normal);*/
	//发送登录完成
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_LOGON,SUB_GP_LOGON_FINISH);

	/*sprintf(info, _T("OnDBLogonSuccess SUB_GP_LOGON_FINISH"));
	CTraceService::TraceString(info, TraceLevel_Normal);*/
	//关闭连接
	if ( pDBRLogonSuccess->dwCustomFaceVer == 0 )
		m_pITCPNetworkEngine->ShutDownSocket(dwContextID);
	
	__LEAVE_FUNCTION
	return true;
}


bool CAttemperEngineSink::OnUpdateUserInfoSucc(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	__ENTER_FUNCTION
	//效验参数
	ASSERT(wDataSize==sizeof(CMD_GP_UserInfoSucc));
	if (wDataSize!=sizeof(CMD_GP_UserInfoSucc)) return false;

	m_pITCPNetworkEngine->SendData(dwContextID, MDM_GP_USER, SUB_GP_UPDATE_USERINFO_SUCC, //发送等级信息
		pData, sizeof(CMD_GP_UserInfoSucc));
	//关闭连接
	m_pITCPNetworkEngine->ShutDownSocket(dwContextID);

	__LEAVE_FUNCTION
	return true;
}

//用户更新失败
bool CAttemperEngineSink::OnUpdateUserInfoFail(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	__ENTER_FUNCTION

	m_pITCPNetworkEngine->ShutDownSocket(dwContextID);

	__LEAVE_FUNCTION
	return true;
}



//用户登录失败
bool CAttemperEngineSink::OnDBLogonError(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	__ENTER_FUNCTION


	//效验参数
	ASSERT(wDataSize==sizeof(DBR_GP_LogonError));
	if (wDataSize!=sizeof(DBR_GP_LogonError)) return false;

	//判断在线
	ASSERT(LOWORD(dwContextID)<m_pInitParamter1->m_wMaxConnect);
	if ((m_pBindParameter+LOWORD(dwContextID))->dwSocketID!=dwContextID) return true;

	//变量定义
	CMD_GP_LogonError LogonError;
	DBR_GP_LogonError * pLogonError=(DBR_GP_LogonError *)pData;
	pLogonError->szErrorDescribe[CountArray(pLogonError->szErrorDescribe)-1]=0;

	//构造数据
	LogonError.lErrorCode=pLogonError->lErrorCode;
	lstrcpyn(LogonError.szErrorDescribe,pLogonError->szErrorDescribe,CountArray(LogonError.szErrorDescribe));
	WORD wDescribeSize=CountStringBuffer(LogonError.szErrorDescribe);

	//发送数据
	WORD wSendSize=sizeof(LogonError)-sizeof(LogonError.szErrorDescribe)+wDescribeSize;
	m_pITCPNetworkEngine->SendData(dwContextID,MDM_GP_LOGON,SUB_GP_LOGON_ERROR,&LogonError,wSendSize);

	//关闭连接
	m_pITCPNetworkEngine->ShutDownSocket(dwContextID);

	__LEAVE_FUNCTION
	return true;
}

//下载成功
bool CAttemperEngineSink::OnDBDownloadFaceSuccess(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	__ENTER_FUNCTION
	//数据转换
	DBR_GP_DownloadFaceSuccess *pDownloadFaceSuccess = (DBR_GP_DownloadFaceSuccess*)pData;

	//数据验证
	WORD wPostSize = WORD(sizeof(DBR_GP_DownloadFaceSuccess)-sizeof(pDownloadFaceSuccess->bFaceData)+pDownloadFaceSuccess->dwCurrentSize);
	ASSERT(wDataSize == wPostSize);
	if ( wPostSize != wDataSize ) return false;

	//发送数据
	CMD_GP_DownloadFaceSuccess DownloadFaceSuccess;
	DownloadFaceSuccess.dwUserID = pDownloadFaceSuccess->dwUserID;
	DownloadFaceSuccess.dwToltalSize = pDownloadFaceSuccess->dwToltalSize;
	DownloadFaceSuccess.dwCurrentSize = pDownloadFaceSuccess->dwCurrentSize;
	CopyMemory(DownloadFaceSuccess.bFaceData, pDownloadFaceSuccess->bFaceData, pDownloadFaceSuccess->dwCurrentSize);
	m_pITCPNetworkEngine->SendData(dwContextID, MDM_GP_USER, SUB_GP_USER_DOWNLOAD_FACE, &DownloadFaceSuccess, wPostSize);

	__LEAVE_FUNCTION
	return true;
}

//成功消息
bool CAttemperEngineSink::OnDBRCustomFaceUploadResult(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	__ENTER_FUNCTION

	ASSERT(wDataSize == sizeof(DBR_GP_UploadFaceResult));
	if ( wDataSize != sizeof(DBR_GP_UploadFaceResult)) return false;

	//类型转换
	DBR_GP_UploadFaceResult *pUploadFaceResult = (DBR_GP_UploadFaceResult*)pData;
	pUploadFaceResult->szDescribeMsg[CountArray(pUploadFaceResult->szDescribeMsg)-1] = 0;

	//设置数据
	CMD_GP_UploadFaceResult UploadFaceResult;
	ZeroMemory(&UploadFaceResult, sizeof(UploadFaceResult));
	lstrcpy(UploadFaceResult.szDescribeMsg, pUploadFaceResult->szDescribeMsg);
	UploadFaceResult.dwFaceVer = pUploadFaceResult->dwFaceVer;
	UploadFaceResult.bOperateSuccess=pUploadFaceResult->bOperateSuccess;

	//发送数据
	m_pITCPNetworkEngine->SendData(dwContextID, MDM_GP_USER, SUB_GP_UPLOAD_FACE_RESULT, &UploadFaceResult, sizeof(UploadFaceResult));

	//关闭链接
	m_pITCPNetworkEngine->ShutDownSocket(dwContextID);

	__LEAVE_FUNCTION
	return true;
}

//删除结果
bool CAttemperEngineSink::OnDBRCustomFaceDeleteResult(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	__ENTER_FUNCTION

	//数据验证
	ASSERT(wDataSize == sizeof(DBR_GP_DeleteFaceResult));
	if (wDataSize != sizeof(DBR_GP_DeleteFaceResult)) return false;

	//类型转换
	DBR_GP_DeleteFaceResult *pDeleteFaceResult = (DBR_GP_DeleteFaceResult*)pData;
	pDeleteFaceResult->szDescribeMsg[CountArray(pDeleteFaceResult->szDescribeMsg)-1] = 0;

	//设置数据
	CMD_GP_DeleteFaceResult DeleteFaceResult;
	ZeroMemory(&DeleteFaceResult, sizeof(DeleteFaceResult));
	lstrcpy(DeleteFaceResult.szDescribeMsg, pDeleteFaceResult->szDescribeMsg);
	DeleteFaceResult.dwFaceVer=pDeleteFaceResult->dwFaceVer;
	DeleteFaceResult.bOperateSuccess = pDeleteFaceResult->bOperateSuccess;

	//发送数据
	m_pITCPNetworkEngine->SendData(dwContextID, MDM_GP_USER, SUB_GP_DELETE_FACE_RESULT, &DeleteFaceResult, sizeof(DeleteFaceResult));

	//关闭链接
	m_pITCPNetworkEngine->ShutDownSocket(dwContextID);

	__LEAVE_FUNCTION

	return true;
}

//修改结果
bool CAttemperEngineSink::OnDBRModifyIndividual(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	__ENTER_FUNCTION
	//数据验证
	ASSERT(wDataSize == sizeof(DBR_GP_ModifyIndividualResult));
	if (wDataSize != sizeof(DBR_GP_ModifyIndividualResult)) return false;

	//类型转换
	DBR_GP_ModifyIndividualResult *pModifyIndividualResul = (DBR_GP_ModifyIndividualResult*)pData;
	pModifyIndividualResul->szDescribeMsg[CountArray(pModifyIndividualResul->szDescribeMsg)-1] = 0;

	CMD_GP_ModifyIndividualResult ModifyIndividualResult;
	ZeroMemory(&ModifyIndividualResult, sizeof(ModifyIndividualResult));
	ModifyIndividualResult.bOperateSuccess = pModifyIndividualResul->bOperateSuccess;
	lstrcpyn(ModifyIndividualResult.szDescribeMsg, pModifyIndividualResul->szDescribeMsg, sizeof(ModifyIndividualResult.szDescribeMsg));

	//发送数据
	m_pITCPNetworkEngine->SendData(dwContextID, MDM_GP_USER, SUB_GP_MODIFY_INDIVIDUAL_RESULT, &ModifyIndividualResult, sizeof(ModifyIndividualResult));

	//关闭链接
	m_pITCPNetworkEngine->ShutDownSocket(dwContextID);

	__LEAVE_FUNCTION

	return true;
}

bool CAttemperEngineSink::OnDBRGradeInfoNum(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	__ENTER_FUNCTION

	if (wDataSize != sizeof(LONG))
	{
		return false;
	}	

	LONG num = *((LONG*)pData);

	m_pGradeInfo = new CMD_GP_UserGradeInfo[num];
	memset(m_pGradeInfo, 0, sizeof(CMD_GP_UserGradeInfo) * num);
	m_iGradeNum  = num;

	__LEAVE_FUNCTION

	return true;
}

bool CAttemperEngineSink::OnDBRGradeInfoOne(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	__ENTER_FUNCTION

	if (wDataSize != sizeof(DBR_GR_gradeInfo))
	{
		return false;
	}

	DBR_GR_gradeInfo* pGradeInfo = (DBR_GR_gradeInfo*)pData;
	if (pGradeInfo->id <= m_iGradeNum )
	{
		int index = pGradeInfo->id  - 1;
		m_pGradeInfo[index].iMaxExp = pGradeInfo->iMaxExp;
		memcpy(m_pGradeInfo[index].strName,  pGradeInfo->szName, sizeof(m_pGradeInfo[index].strName));
		m_pGradeInfo[index].strName[sizeof(m_pGradeInfo[index].strName) - 1] = 0;
	}

	__LEAVE_FUNCTION
	return true;
}

//返回免费领取的结果
bool CAttemperEngineSink::OnDBRGiftMoneyResult(DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	__ENTER_FUNCTION

	if (wDataSize != sizeof(DBR_GP_GiftGoldResult))
	{
		return false;
	}

	//发送数据
	m_pITCPNetworkEngine->SendData(dwContextID, MDM_GP_USER, 
		SUB_GP_GET_GIFT_GOLD_RESULT , pData, wDataSize);
	//关闭链接
	m_pITCPNetworkEngine->ShutDownSocket(dwContextID);

	__LEAVE_FUNCTION

	return true;
}

//连接事件
bool __cdecl CAttemperEngineSink::OnEventTCPSocketLink(WORD wServiceID, INT nErrorCode)
{
	__ENTER_FUNCTION

	//错误判断
	if (nErrorCode!=0)
	{
		//设置时间
		m_pITimerEngine->SetTimer(IDI_CONNECT_CENTER_SERVER,TIME_RECONNECT,1,NULL);

		//错误提示
		CTraceService::TraceString(TEXT("中心服务器连接失败，稍后将会再次尝试...."),TraceLevel_Warning);

		return true;
	}
	else
	{
		//错误提示
		CTraceService::TraceString(TEXT("中心服务器连接成功"),TraceLevel_Normal);
	}

	//获取列表
	m_pITCPSocketCorrespond->SendData(MDM_CS_SERVER_LIST,SUB_CS_GET_SERVER_LIST,NULL,0);

	//设置定时器
	m_pITimerEngine->SetTimer(IDI_UPDATE_SERVER_LIST,TIME_UPDATE_LIST,TIMES_INFINITY,NULL);

	__LEAVE_FUNCTION

	return true;
}

//关闭事件
bool __cdecl CAttemperEngineSink::OnEventTCPSocketShut(WORD wServiceID, BYTE cbShutReason)
{
	__ENTER_FUNCTION

	//重新连接
	m_pITimerEngine->KillTimer(IDI_UPDATE_SERVER_LIST);
	m_pITimerEngine->SetTimer(IDI_CONNECT_CENTER_SERVER,TIME_RECONNECT,1,NULL);

	//错误提示
	CTraceService::TraceString(TEXT("中心服务器连接中断，稍后将会再次尝试...."),TraceLevel_Warning);

	__LEAVE_FUNCTION

	return true;
}

//读取事件
bool __cdecl CAttemperEngineSink::OnEventTCPSocketRead(WORD wServiceID, CMD_Command Command, VOID * pData, WORD wDataSize)
{
	switch (Command.wMainCmdID)
	{
	case MDM_CS_SERVER_LIST:	//列表消息
		{
			return OnCenterMainServerList(Command.wSubCmdID,pData,wDataSize);
		}
	}

	return true;
}

//网络应答事件
bool __cdecl CAttemperEngineSink::OnEventTCPNetworkBind(DWORD dwClientIP, DWORD dwSocketID)
{
	__ENTER_FUNCTION

	//获取索引
	ASSERT(LOWORD(dwSocketID)<m_pInitParamter1->m_wMaxConnect);
	if (LOWORD(dwSocketID)>=m_pInitParamter1->m_wMaxConnect) return false;

	//变量定义
	WORD wBindIndex=LOWORD(dwSocketID);
	tagBindParameter * pBindParameter=(m_pBindParameter+wBindIndex);

	//设置变量
	pBindParameter->dwSocketID=dwSocketID;
	pBindParameter->dwClientIP=dwClientIP;
	pBindParameter->dwActiveTime=(DWORD)time(NULL);

	__LEAVE_FUNCTION

	return true;
}

//网络读取事件
bool __cdecl CAttemperEngineSink::OnEventTCPNetworkRead(CMD_Command Command, VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	bool ret = false;
	switch (Command.wMainCmdID)
	{
	case MDM_GP_LOGON:				//登录消息
		{
			ret = OnSocketMainLogon(Command.wSubCmdID,pData,wDataSize,dwSocketID);
			break;
		} 
	case MDM_GP_USER:				//用户信息
		{
			ret = OnSocketMainUser(Command.wSubCmdID,pData,wDataSize,dwSocketID);
			break;
		}
	}

	if (!ret)
	{
		char info[BUFSIZ] = {0};
		sprintf(info, _T("OnEventTCPNetworkRead %i %i"), Command.wMainCmdID, Command.wSubCmdID);
		CTraceService::TraceString(info, TraceLevel_Normal);
	}
	return ret;
}

//网络关闭事件
bool __cdecl CAttemperEngineSink::OnEventTCPNetworkShut(DWORD dwClientIP, DWORD dwActiveTime, DWORD dwSocketID)
{
	//清除信息
	WORD wBindIndex=LOWORD(dwSocketID);
	ZeroMemory((m_pBindParameter+wBindIndex),sizeof(tagBindParameter));

	/*char info[BUFSIZ] = {0};
	sprintf(info, _T("OnEventTCPNetworkShut"));
	CTraceService::TraceString(info, TraceLevel_Normal);*/
	return true;
}

//登录消息处理
bool CAttemperEngineSink::OnSocketMainLogon(WORD wSubCmdID, VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	__ENTER_FUNCTION

	switch (wSubCmdID)
	{
	case SUB_GP_LOGON_ACCOUNTS:		//帐号登录
		{
			//效验参数
			/*char info[BUFSIZ] = {0};
			sprintf(info, _T("OnSocketMainLogon"));
			CTraceService::TraceString(info, TraceLevel_Normal);*/

			ASSERT(wDataSize>=sizeof(CMD_GP_LogonByAccounts));
			if (wDataSize<sizeof(CMD_GP_LogonByAccounts)) return true;

			//处理消息
			CMD_GP_LogonByAccounts * pLogonByAccounts=(CMD_GP_LogonByAccounts *)pData;
			pLogonByAccounts->szAccounts[CountArray(pLogonByAccounts->szAccounts)-1]=0;
			pLogonByAccounts->szPassWord[CountArray(pLogonByAccounts->szPassWord)-1]=0;

			/*sprintf(info, _T("OnSocketMainLogon %s"), pLogonByAccounts->szAccounts);
			CTraceService::TraceString(info, TraceLevel_Normal);*/

			//连接信息
			ASSERT(LOWORD(dwSocketID)<m_pInitParamter1->m_wMaxConnect);
			DWORD dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientIP;

			////效验版本
			//if (pLogonByAccounts->dwPlazaVersion!=VER_PLAZA_FRAME)
			//{
			//	//获取版本
			//	WORD wLow=LOWORD(pLogonByAccounts->dwPlazaVersion);
			//	WORD wHigh=HIWORD(pLogonByAccounts->dwPlazaVersion);

			//	//构造数据
			//	CMD_GP_Version Version;
			//	Version.bNewVersion=TRUE;
			//	Version.bAllowConnect=(wHigh==VER_PLAZA_HIGH)?TRUE:FALSE;

			//	//发送数据
			//	m_pITCPNetworkEngine->SendData(dwSocketID,MDM_GP_SYSTEM,SUB_GP_VERSION,&Version,sizeof(Version));

			//	//判断关闭
			//	//if (Version.bAllowConnect==FALSE)
			//	//{
			//	//	return true;
			//	//}
			//	sprintf(info, _T("OnSocketMainLogon  SUB_GP_VERSION"));
			//	CTraceService::TraceString(info, TraceLevel_Normal);
			//	return true;
			//}

			//构造数据
			DBR_GP_LogonByAccounts LogonByAccounts;
			ZeroMemory(&LogonByAccounts,sizeof(LogonByAccounts));

			//设置变量
			LogonByAccounts.dwClientIP=dwClientAddr;
			lstrcpyn(LogonByAccounts.szAccounts,pLogonByAccounts->szAccounts,sizeof(LogonByAccounts.szAccounts));
			lstrcpyn(LogonByAccounts.szPassWord,pLogonByAccounts->szPassWord,sizeof(LogonByAccounts.szPassWord));

			//变量定义
			VOID * pData=NULL;
			tagDataDescribe DataDescribe;
			CRecvPacketHelper RecvPacket(pLogonByAccounts+1,wDataSize-sizeof(CMD_GP_LogonByAccounts));

			//扩展信息
			while (true)
			{
				//提取数据
				pData=RecvPacket.GetData(DataDescribe);
				if (DataDescribe.wDataDescribe==DTP_NULL) break;

				//数据分析
				switch (DataDescribe.wDataDescribe)
				{
				case DTP_COMPUTER_ID:		//机器标识
					{
						ASSERT(pData!=NULL);
						ASSERT(DataDescribe.wDataSize==sizeof(tagClientSerial));
						if (DataDescribe.wDataSize==sizeof(tagClientSerial))
						{
							BYTE * pcbByte=(BYTE *)pData;
							for (INT i=0;i<sizeof(tagClientSerial)/sizeof(BYTE);i++)
							{
								ASSERT(CountArray(LogonByAccounts.szComputerID)>((i*2)+1));
								_stprintf(&LogonByAccounts.szComputerID[i*2],TEXT("%02X"),*(pcbByte+i));
							}
						}

						break;
					}
				}
			}

			/*sprintf(info, _T("OnSocketMainLogon  DBR_GP_LOGON_BY_ACCOUNTS"));
			CTraceService::TraceString(info, TraceLevel_Normal);*/

			//投递数据库
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_LOGON_BY_ACCOUNTS,dwSocketID,&LogonByAccounts,sizeof(LogonByAccounts));

			return true;
		}
	}

	__LEAVE_FUNCTION

	return true;
}

//用户信息处理
bool CAttemperEngineSink::OnSocketMainUser(WORD wSubCmdID, VOID * pData, WORD wDataSize, DWORD dwSocketID)
{
	__ENTER_FUNCTION

	switch (wSubCmdID)
	{
	case SUB_GP_USER_UPLOAD_FACE:		//上传头像
		{
			//数据转换
			CMD_GP_UploadCustomFace *pUploadCustomFace = (CMD_GP_UploadCustomFace*)pData;

			//数据验证
			ASSERT(wDataSize == sizeof(CMD_GP_UploadCustomFace)-sizeof(pUploadCustomFace->bFaceData)+pUploadCustomFace->wCurrentSize);
			if (wDataSize != sizeof(CMD_GP_UploadCustomFace)-sizeof(pUploadCustomFace->bFaceData)+pUploadCustomFace->wCurrentSize) return false;

			//写入文件
			try
			{
				//文件定义
				CFile fileCustomFace;
				TCHAR szFileName[128];
				_snprintf(szFileName, CountArray(szFileName), TEXT("%s\\UploadFile_%ld.zip"), theApp.m_szDirWork, pUploadCustomFace->dwUserID);
				BOOL bSuccess=FALSE;
				if ( pUploadCustomFace->bFirstPacket == true )
					bSuccess=fileCustomFace.Open( szFileName, CFile::modeCreate|CFile::modeWrite|CFile::typeBinary);
				else
					bSuccess=fileCustomFace.Open( szFileName, CFile::modeCreate|CFile::modeWrite|CFile::typeBinary|CFile::modeNoTruncate);
				if ( bSuccess )
				{
					//移动指针
					fileCustomFace.SeekToEnd();

					//写入文件
					fileCustomFace.Write(pUploadCustomFace->bFaceData, pUploadCustomFace->wCurrentSize);

					//关闭文件
					fileCustomFace.Close();

					//完成判断
					if ( pUploadCustomFace->bLastPacket == true )
					{
						//投递请求
						DBR_GP_UploadCustomFace UploadCustomFace;
						ZeroMemory(&UploadCustomFace, sizeof(UploadCustomFace));
						UploadCustomFace.dwUserID = pUploadCustomFace->dwUserID;
						m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_USER_UPLOAD_FACE, dwSocketID, &UploadCustomFace, sizeof(DBR_GP_UploadCustomFace));
					}
				}
			}
			catch(...){}

			return true;
		}
	case SUB_GP_USER_DOWNLOAD_FACE:		//下载头像
		{
			//数据验证
			ASSERT(sizeof(CMD_GP_DownloadFace)==wDataSize);
			if ( sizeof(CMD_GP_DownloadFace) != wDataSize) return false;

			CMD_GP_DownloadFace *pDownloadFace = (CMD_GP_DownloadFace*)pData;

			ASSERT( pDownloadFace->dwUserID != 0 );

			//投递请求
			DBR_GP_DownloadCustomFace DownloadCustomFace;
			ZeroMemory(&DownloadCustomFace, sizeof(DownloadCustomFace));
			DownloadCustomFace.dwUserID = pDownloadFace->dwUserID ;
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_DOWNLOAD_FACE, dwSocketID, &DownloadCustomFace, sizeof(DownloadCustomFace));

			return true;
		}
	case SUB_GP_CUSTOM_FACE_DELETE:		//删除头像
		{
			//数据验证
			ASSERT(sizeof(CMD_GP_CustomFaceDelete) == wDataSize);
			if ( sizeof(CMD_GP_CustomFaceDelete) != wDataSize) return false;

			//类型转换
			CMD_GP_CustomFaceDelete *pCustomFaceDelete = (CMD_GP_CustomFaceDelete *)pData;

			DBR_GP_CustomFaceDelete CustomFaceDelete;
			ZeroMemory(&CustomFaceDelete, sizeof(CustomFaceDelete));
			CustomFaceDelete.dwUserID = pCustomFaceDelete->dwUserID;
			CustomFaceDelete.dwFaceVer = pCustomFaceDelete->dwFaceVer;

			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_DELETE_FACE, dwSocketID, &CustomFaceDelete, sizeof(CustomFaceDelete));

			return true;
		}
	case SUB_GP_MODIFY_INDIVIDUAL:		//修改资料
		{
			//数据验证
			ASSERT(sizeof(CMD_GP_ModifyIndividual) == wDataSize);
			if ( sizeof(CMD_GP_ModifyIndividual) != wDataSize) return false;

			//类型转换
			CMD_GP_ModifyIndividual *pModifyIndividual = (CMD_GP_ModifyIndividual *)pData;
			pModifyIndividual->szAddress[sizeof(pModifyIndividual->szAddress)-1] = 0;
			pModifyIndividual->szNickname[sizeof(pModifyIndividual->szNickname)-1] = 0;
			pModifyIndividual->szPassword[sizeof(pModifyIndividual->szPassword)-1] = 0;
			pModifyIndividual->szUnderWrite[sizeof(pModifyIndividual->szUnderWrite)-1] = 0;

			DBR_GP_ModifyIndividual ModifyIndividual;

			ZeroMemory(&ModifyIndividual, sizeof(ModifyIndividual));

			ModifyIndividual.dwUserID = pModifyIndividual->dwUserID;
			ModifyIndividual.nAge = pModifyIndividual->nAge;
			ModifyIndividual.nGender = pModifyIndividual->nGender;
			lstrcpyn(ModifyIndividual.szAddress, pModifyIndividual->szAddress, sizeof(ModifyIndividual.szAddress));
			lstrcpyn(ModifyIndividual.szNickname, pModifyIndividual->szNickname, sizeof(ModifyIndividual.szNickname));
			lstrcpyn(ModifyIndividual.szPassword, pModifyIndividual->szPassword, sizeof(ModifyIndividual.szPassword));
			lstrcpyn(ModifyIndividual.szUnderWrite, pModifyIndividual->szUnderWrite, sizeof(ModifyIndividual.szUnderWrite));

			//投递请求
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_MODIFY_INDIVIDUAL, dwSocketID, &ModifyIndividual, sizeof(ModifyIndividual));

			return true;
		}
	case SUB_GP_UPDATE_USERINFO :       //查询用户信息
		{
			//效验参数
			if (wDataSize != sizeof(CMD_GP_UpdateUserInfo))
				return false;

			//构造数据
			DBR_GP_UpdateUserInfo updateUserInfo;
			memcpy(&updateUserInfo, pData, sizeof(updateUserInfo));

			//投递数据库
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_UPDATE_USERINFO,dwSocketID,&updateUserInfo, sizeof(updateUserInfo));
			return true;
		}
	case SUB_GP_GET_GIFT_GOLD:     //每天一次的免费领取
		{
			//效验参数
			if (wDataSize != sizeof(CMD_GP_GiftGold))
				return false;

			//构造数据
			DBR_GP_GiftGold   giftMoney;
			memcpy(&giftMoney, pData, sizeof(giftMoney));
			//连接信息
			ASSERT(LOWORD(dwSocketID)<m_pInitParamter1->m_wMaxConnect);
			DWORD dwClientAddr=(m_pBindParameter+LOWORD(dwSocketID))->dwClientIP;
			giftMoney.dwClientIP=dwClientAddr;
			//投递数据库
			m_pIDataBaseEngine->PostDataBaseRequest(DBR_GP_GET_GIFT_GLOD,dwSocketID,&giftMoney, sizeof(giftMoney));
			return true;
		}
	case SUB_GP_CHECK_LINE:		// 检查线路
		{
			//效验参数
			if (wDataSize != sizeof(CMD_GP_CheckLine))
				return false;

			CMD_GP_CheckLine * pCheckLineData = (CMD_GP_CheckLine *)pData;

			//构造数据
			CMD_GP_CheckLine checkline;
			memcpy(&checkline, pCheckLineData, sizeof(checkline));
			//连接信息
			ASSERT(LOWORD(dwSocketID)<m_pInitParamter1->m_wMaxConnect);
			checkline.dwClientIP = (m_pBindParameter+LOWORD(dwSocketID))->dwClientIP;
			//发送数据
			m_pITCPNetworkEngine->SendData(dwSocketID, MDM_GP_USER, SUB_GP_CHECK_LINE, &checkline, sizeof(checkline));
			//关闭链接
			m_pITCPNetworkEngine->ShutDownSocket(dwSocketID);
			return true;
		}
	case SUB_GP_DELETE_MACHINE_CODE:	//删除机器码
		{
#if !defined(_DEBUG)
			//效验参数
			if (wDataSize != sizeof(CMD_GP_DeleteMachineCode))
				return false;

			CMD_GP_DeleteMachineCode * pDeleteMachineCode = (CMD_GP_DeleteMachineCode *)pData;
			BYTE * pcbByte=(BYTE *)pDeleteMachineCode;
			TCHAR	szComputerID[COMPUTER_ID_LEN];		//机器序列

			for (INT i=0;i<sizeof(tagClientSerial)/sizeof(BYTE);i++)
			{
				ASSERT(CountArray(szComputerID)>((i*2)+1));
				_stprintf(&szComputerID[i*2],TEXT("%02X"),*(pcbByte+i));
			}

			MMachine::iterator M_const_return = m_hmMachineCode.find(szComputerID);
			if (m_hmMachineCode.end() != M_const_return)
			{
				if (M_const_return->second->compare(pDeleteMachineCode->szAccounts))
				{
					m_hmMachineCode.erase(M_const_return);
				}
			}
#endif
			//发送数据
			m_pITCPNetworkEngine->SendData(dwSocketID, MDM_GP_USER, SUB_GP_DELETE_MACHINE_CODE);
			//关闭链接
			m_pITCPNetworkEngine->ShutDownSocket(dwSocketID);

			return true;
		}
	default:
		char info[BUFSIZ] = {0};
		{
			sprintf(info, _T("OnSocketMainUser, 无用的消息"));
			CTraceService::TraceString(info, TraceLevel_Normal);
			ASSERT(TRUE);
			return true;
		}
	}

	__LEAVE_FUNCTION

	return true;
}



//列表消息处理
bool CAttemperEngineSink::OnCenterMainServerList(WORD wSubCmdID, VOID * pData, WORD wDataSize)
{
	__ENTER_FUNCTION

	switch (wSubCmdID)
	{
	case SUB_CS_LIST_INFO:		//列表信息
		{
			//效验参数
			ASSERT(wDataSize==sizeof(CMD_CS_ListInfo));
			if (wDataSize!=sizeof(CMD_CS_ListInfo))
				return false;

			//消息处理
			m_ServerList.ResetServerListBuffer();

			return true;
		}
	case SUB_CS_LIST_TYPE:		//种类信息
		{
			//效验参数
			ASSERT(wDataSize>=sizeof(tagGameType));
			ASSERT(wDataSize%sizeof(tagGameType)==0);

			//消息处理
			DWORD dwCount=wDataSize/sizeof(tagGameType);
			tagGameType * pGameType=(tagGameType *)pData;
			m_ServerList.AppendGameTypeBuffer(pGameType,dwCount);

			return true;
		}
	case SUB_CS_LIST_KIND:		//类型信息
		{
			//效验参数
			ASSERT(wDataSize>=sizeof(tagGameKind));
			ASSERT(wDataSize%sizeof(tagGameKind)==0);

			//消息处理
			DWORD dwCount=wDataSize/sizeof(tagGameKind);
			tagGameKind * pGameKind=(tagGameKind *)pData;
			m_ServerList.AppendGameKindBuffer(pGameKind,dwCount);

			return true;
		}
	case SUB_CS_LIST_STATION:	//站点信息
		{
			//效验参数
			ASSERT(wDataSize>=sizeof(tagGameStation));
			ASSERT(wDataSize%sizeof(tagGameStation)==0);

			//消息处理
			DWORD dwCount=wDataSize/sizeof(tagGameStation);
			tagGameStation * pGameStation=(tagGameStation *)pData;
			m_ServerList.AppendGameStationBuffer(pGameStation,dwCount);

			return true;
		}
	case SUB_CS_LIST_SERVER:	//房间信息
		{
			//效验参数
			ASSERT(wDataSize>=sizeof(tagGameServer));
			ASSERT(wDataSize%sizeof(tagGameServer)==0);

			//消息处理
			DWORD dwCount=wDataSize/sizeof(tagGameServer);
			tagGameServer * pGameServer=(tagGameServer *)pData;
			m_ServerList.AppendGameServerBuffer(pGameServer,dwCount);

			return true;
		}
	case SUB_CS_LIST_FINISH:	//更新完成
		{
			//消息处理
			m_ServerList.ActiveServerListBuffer();

			return true;
		}
	}

	__LEAVE_FUNCTION

	return true;
}

//////////////////////////////////////////////////////////////////////////
