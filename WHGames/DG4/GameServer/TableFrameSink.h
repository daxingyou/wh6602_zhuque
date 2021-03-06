#ifndef TABLE_FRAME_SINK_HEAD_FILE
#define TABLE_FRAME_SINK_HEAD_FILE

#pragma once


#include <afxwin.h>         // MFC 核心组件和标准组件
#include <afxext.h>         // MFC 扩展

//#include "GameLogic.h"
#include "../XY/CMD_DG4.h"
#include "GameServiceExport.h"

#include <string>
#include <vector>
using namespace std;
//////////////////////////////////////////////////////////////////////////

#define GAME_TIMER				1


//游戏桌子类
class CTableFrameSink : public ITableFrameSink, public ITableUserAction
{

	//玩家变量
protected:
	WORD							m_wDUser;								//D玩家
	WORD							m_wCurrentUser;							//当前玩家

	//玩家状态
protected:
	unsigned char							m_cbPlayStatus[GAME_PLAYER];			//游戏状态

	//组件变量
protected:
//	CGameLogic						m_GameLogic;							//游戏逻辑
	ITableFrame						* m_pITableFrame;						//框架接口
	const tagGameServiceOption		* m_pGameServiceOption;					//配置参数

	//属性变量
protected:
	static const WORD				m_wPlayerCount;							//游戏人数
	static const enStartMode		m_GameStartMode;						//开始模式

protected:
	virtual void OnStartGame() {};
	virtual void OnGameAction(WORD wSubCmdID, const void * pDataBuffer, WORD wDataSize) {};

	bool SendTableData(WORD wChairID, WORD wSubCmdID, void * pData, WORD wDataSize);
	bool SendTableDataToAll(WORD wSubCmdID, void * pData, WORD wDataSize);
	void SendTableDataToOther(WORD wChairID, WORD wSubCmdID, void * pData, WORD wDataSize);

	IServerUserItem * GetServerUserItem(WORD wChairID);

	void stovs( const string& s, const string& sp, vector<string>& sa );
	//函数定义
public:
	//构造函数
	CTableFrameSink();
	//析构函数
	virtual ~CTableFrameSink();

	//基础接口
public:
	//释放对象
	virtual VOID __cdecl Release() { }
	//是否有效
	virtual bool __cdecl IsValid() { return AfxIsValidAddress(this,sizeof(CTableFrameSink))?true:false; }
	//接口查询
	virtual void * __cdecl QueryInterface(const IID & Guid, DWORD dwQueryVer);

	//管理接口
public:
	//初始化
	virtual bool __cdecl InitTableFrameSink(IUnknownEx * pIUnknownEx);
	//复位桌子
	virtual void __cdecl RepositTableFrameSink();

	//信息接口
public:
	//开始模式
	virtual enStartMode __cdecl GetGameStartMode();
	//游戏状态
	virtual bool __cdecl IsUserPlaying(WORD wChairID);

	//动作事件 
public:
	//用户断线
	virtual bool __cdecl OnActionUserOffLine(WORD wChairID, IServerUserItem * pIServerUserItem) { return true; }
	//用户重入
	virtual bool __cdecl OnActionUserReConnect(WORD wChairID, IServerUserItem * pIServerUserItem) { return true; }
	//用户坐下
	virtual bool __cdecl OnActionUserSitDown(WORD wChairID, IServerUserItem * pIServerUserItem, bool bLookonUser);
	//用户起来
	virtual bool __cdecl OnActionUserStandUp(WORD wChairID, IServerUserItem * pIServerUserItem, bool bLookonUser);
	//用户同意
	virtual bool __cdecl OnActionUserReady(WORD wChairID, IServerUserItem * pIServerUserItem, VOID * pData, WORD wDataSize) { return true; }

	//游戏事件
public:
	//游戏开始
	virtual bool __cdecl OnEventGameStart();
	//游戏结束
	virtual bool __cdecl OnEventGameEnd(WORD wChairID, IServerUserItem * pIServerUserItem, unsigned char cbReason);
	//发送场景
	virtual bool __cdecl SendGameScene(WORD wChiarID, IServerUserItem * pIServerUserItem, unsigned char cbGameStatus, bool bSendSecret);

	//消息事件
protected:
	//放弃事件
	bool OnUserGiveUp(WORD wChairID);
	//加注事件
	bool OnUserAddScore(WORD wChairID, LONG lScore, bool bGiveUp);

	//事件接口
public:
	//定时器事件
	virtual bool __cdecl OnTimerMessage(WORD wTimerID, WPARAM wBindParam);
	//游戏消息处理
	virtual bool __cdecl OnGameMessage(WORD wSubCmdID, const void * pDataBuffer, WORD wDataSize, IServerUserItem * pIServerUserItem);
	//框架消息处理
	virtual bool __cdecl OnFrameMessage(WORD wSubCmdID, const void * pDataBuffer, WORD wDataSize, IServerUserItem * pIServerUserItem);
};

//////////////////////////////////////////////////////////////////////////

#endif