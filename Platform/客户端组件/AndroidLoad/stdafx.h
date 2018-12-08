#pragma once

//////////////////////////////////////////////////////////////////////////

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#ifndef WINVER
#define WINVER 0x0501
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif						

#ifndef _WIN32_WINDOWS	
#define _WIN32_WINDOWS 0x0501
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x0501
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS

#define _AFX_ALL_WARNINGS

#include <afxwin.h> 
#include <afxext.h> 
#include <afxdisp.h>

#include <afxdtctl.h>
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>
#endif

//////////////////////////////////////////////////////////////////////////

//系统头文件
#include "Nb30.h"
#include "Afxmt.h"
#include "AfxHtml.h"
#include "AfxInet.h"


//全局头文件
#include "..\..\公共文件\Constant.h"
#include "..\..\公共文件\GlobalDef.h"
#include "..\..\公共文件\GlobalField.h"
#include "..\..\公共文件\GlobalFrame.h"
#include "..\..\公共文件\GlobalRight.h"

//命令头文件
#include "..\..\消息定义\CMD_Game.h"
#include "..\..\消息定义\CMD_Plaza.h"
#include "..\..\消息定义\CMD_Video.h"

//模板库
#include "..\..\模板库\Template.h"

//组件头文件
#include "..\网络服务\NetworkServiceHead.h"
#include "..\..\共享组件\公共服务\ComService.h"



//////////////////////////////////////////////////////////////////////////
