#pragma once

#include "Stdafx.h"
#include "GameLogic.h"
#include "GameClientView.h"
#include "GameScoreView.h"

//////////////////////////////////////////////////////////////////////////

//游戏对话框
class CGameClientDlg : public CGameFrameDlg
{
	//游戏变量
protected:
	WORD							m_wLandUser;						//挖坑用户
	WORD							m_wBombTime;						//炸弹倍数
	BYTE							m_bCardCount[3];					//扑克数目
	BYTE							m_bHandCardCount[3];				//扑克数目
	BYTE							m_bHandCardData[3][20];				//手上扑克
	bool							m_bAutoPlay ;						//托管变量

	//出牌变量
protected:
	BYTE							m_bTurnOutType;						//出牌类型
	BYTE							m_bTurnCardCount;					//出牌数目
	BYTE							m_bTurnCardData[20];				//出牌列表

	//配置变量
protected:
	bool							m_bDeasilOrder;						//出牌顺序
	DWORD							m_dwCardHSpace;						//扑克象素
	bool							m_bAllowLookon;						//允许旁观

	//辅助变量
protected:
	WORD							m_wMostUser;						//最大玩家
	WORD							m_wCurrentUser;						//当前用户
	WORD							m_wTimeOutCount;					//超时次数
	BYTE							m_cbSortType;						//排序类型
	bool                            m_bSystemTrustee;                   //系统托管
	WORD                            m_wLastMostUser;                    //上轮出牌玩家

	//辅助变量
protected:
	BYTE							m_cbRemnantCardCount;				//剩余数目
	BYTE							m_cbDispatchCardCount;				//派发数目
	BYTE							m_cbDispatchCardData[20];			//派发扑克

	//疯狂变量
protected:
	BYTE							m_cbCallScorePhase;					//叫牌阶段

	//控件变量
protected:
	CGameLogic						m_GameLogic;						//游戏逻辑
	CGameClientView					m_GameClientView;					//游戏视图
	CGameScoreView                  m_ScoreView;                        //积分视图


	//春天和反春
protected:
	bool                            m_IsSpring;                          //是否春天
	bool                            m_IsFanSpring;                       //是否反春
	int                             m_LargessCount[GAME_PLAYER];          //宝石数量
	//是否已经播放
	bool                            m_bchuntian;
	bool                            m_bfanchun;
	bool                            m_bbaoshi;
	bool                            m_bjieshu;


	//声音
protected:
	CString							m_sVernacular;						//方言

private:
	tagOutCardResult				m_tagOldCard;
	tagOutCardResult				m_tagNewCard;
	//函数定义
public:
	//构造函数
	CGameClientDlg();
	//析构函数
	virtual ~CGameClientDlg();
	//获取路径
    void GetPicPath(CString& strPath);

	//常规继承
private:
	//初始函数
	virtual bool InitGameFrame();
	//重置框架
	virtual void ResetGameFrame();
	//游戏设置
	virtual void OnGameOptionSet();
	//时间消息
	virtual bool OnTimerMessage(WORD wChairID, UINT nElapse, UINT nTimerID);
	//旁观状态
	virtual void OnLookonChanged(bool bLookonUser, const void * pBuffer, WORD wDataSize);
	//网络消息
	virtual bool OnGameMessage(WORD wSubCmdID, const void * pBuffer, WORD wDataSize);
	//游戏场景
	virtual bool OnGameSceneMessage(BYTE cbGameStatus, bool bLookonOther, const void * pBuffer, WORD wDataSize);

public:
	//用户进入
	virtual void __cdecl OnEventUserEnter(tagUserData * pUserData, WORD wChairID, bool bLookonUser);
	//用户离开
	virtual void __cdecl OnEventUserLeave(tagUserData * pUserData, WORD wChairID, bool bLookonUser);

public:
	//点击继续游戏
	virtual void OnEventContinueGame();

	//消息处理
protected:
	//发送扑克
	bool OnSubSendCard(const void * pBuffer, WORD wDataSize);
	//用户叫地主
	bool OnSubLandScore(const void * pBuffer, WORD wDataSize);
	//用户抢地主
	bool onSubSnatchLand(const void *pBuffer,WORD wDataSize);
	//游戏开始
	bool OnSubGameStart(const void * pBuffer, WORD wDataSize);
	//用户出牌
	bool OnSubOutCard(const void * pBuffer, WORD wDataSize);
	//放弃出牌
	bool OnSubPassCard(const void * pBuffer, WORD wDataSize);
	//游戏结束
	bool OnSubGameEnd(const void * pBuffer, WORD wDataSize);
	//玩家托管
	bool OnSubPlayerTrustee(const void * pBuffer, WORD wDataSize);

	//疯狂消息
protected:
	//明牌开始
	bool OnSubBrightStart(const void * pBuffer, WORD wDataSize);
	//玩家明牌
	bool OnSubBrightCard(const void * pBuffer, WORD wDataSize);
	//加倍消息
	bool OnSubDoubleScore(const void * pBuffer, WORD wDataSize);
	//玩家加倍
	bool OnSubUserDouble(const void * pBuffer, WORD wDataSize);

	//辅助函数
protected:
	//出牌判断
	bool VerdictOutCard();
	//自动出牌
	bool AutomatismOutCard();
	//停止发牌
	bool DoncludeDispatchCard();
	//派发扑克
	bool DispatchUserCard(BYTE cbCardData[], BYTE cbCardCount);

	//声音
protected:
	//播放声音
	void PlayGameSoundLanguage(BYTE cbGender, CString strSound,bool bSoundType);//参数(1:性别;2:声音;3:多种声音)
   // void PlaySound(CString szSoundRes, bool bLoop);
	void PlayMusic(CString szSoundRes);
public:
	void PlaySound(CString szSoundRes, bool bLoop);
	//消息映射
protected:
	//定时器消息
	afx_msg void OnTimer(UINT nIDEvent);
	//开始消息
	LRESULT OnStart(WPARAM wParam, LPARAM lParam);
	//出牌消息
	LRESULT OnOutCard(WPARAM wParam, LPARAM lParam);
	//放弃出牌
	LRESULT OnPassCard(WPARAM wParam, LPARAM lParam);
	//叫地主消息
	LRESULT OnCallLand(WPARAM wParam, LPARAM lParam);
	//抢地主消息
	LRESULT OnSnatchLand(WPARAM wParem,LPARAM lParam);
	//出牌提示
	LRESULT OnAutoOutCard(WPARAM wParam, LPARAM lParam);
	//左键键扑克
	LRESULT OnLeftHitCard(WPARAM wParam, LPARAM lParam);
	//右键扑克
	LRESULT OnRightHitCard(WPARAM wParam, LPARAM lParam);
	//托管消息
	LRESULT OnAutoPlay(WPARAM wParam, LPARAM lParam);
	//排列扑克
	LRESULT OnMessageSortCard(WPARAM wParam, LPARAM lParam);
	
	//疯狂消息
protected:
	//明牌开始
	LRESULT OnBrightStart(WPARAM wParam, LPARAM lParam);
	//加倍消息
	LRESULT OnDoubleScore(WPARAM wParam, LPARAM lParam);
		
	DECLARE_MESSAGE_MAP()

private:
	//不点击PASS按钮,直接出牌。
	void NoHitPassOutCard();

	//统计人数
	int GetCurGameUserNums();
};

//////////////////////////////////////////////////////////////////////////
