// SkillManager.cpp: implementation of the CSkillManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef _MHCLIENT_

#include "SkillManager_client.h"
#include "ActionTarget.h"

#include "Hero.h"
#include "MoveManager.h"
#include "QuickManager.h"
#include "Gamein.h"
#include "QuickItem.h"
#include "ObjectStateManager.h"
#include "ObjectManager.h"
#include "TacticManager.h"

#include "ChatManager.h"
#include "ExchangeManager.h"

#include "GameResourceManager.h"
#include "QuickDialog.h"

#include "PKManager.h"
#include "ObjectActionManager.h"
#include "BattleSystem_Client.h"
#include "../[CC]BattleSystem/GTournament/Battle_GTournament.h"

#include "PeaceWarModeManager.h"

#include "MAINGAME.h"

#include "SkillDelayManager.h"

#include "PeaceWarModeManager.h"
#include "InventoryExDialog.h"
#include "Item.h"
#include "PartyWar.h"
#include "ItemManager.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//GLOBALTON(CSkillManager);
CSkillManager::CSkillManager()
{
	g_PoolSTLIST.Init(100,50,"g_PoolSTLIST");
	m_pSkillInfo = NULL;

	m_HwaKyungSkillTable.Initialize(30);
	m_GeukMaSkillTable.Initialize(30);

	//////////////////////////////////////////////////////////////////////////
	// 06. 06. 2차 전직 - 이영준
	// 무공 변환 추가
	m_SkillOptionTable.Initialize(30);
	m_SkillOptionByItemTable.Initialize(30);
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	// 06. 06. 2차 전직 - 이영준
	// 은신/혜안
	m_SpecialStateInfoTable.Initialize(3);
	//////////////////////////////////////////////////////////////////////////

	// debug용
	m_nSkillUseCount = 0;
}

CSkillManager::~CSkillManager()
{
	Release();
	g_PoolSTLIST.Release();
}

void CSkillManager::Init()
{
	m_SkillInfoTable.Initialize(2000);
//	m_DummySkillInfoTable.Initialize(64);
	m_SkillObjectTable.Initialize(512);
	LoadSkillInfoList();
	LoadSkillChangeInfoList();

	m_SkillAreaMgr.LoadSkillAreaList();

	m_JobSkillProbabilityTable.Initialize(MAX_JOBLEVEL_NUM);	// 2007. 6. 28. CBH - 전문기술 확률 테이블 초기화 /////////////////
	LoadJobSkillProbability();				// 2007. 6. 28. CBH - 전문기술 확률 리소스 로딩 /////////////////

	// debug용
	m_nSkillUseCount = 0;
}
void CSkillManager::Release()
{
	CSkillInfo* pSInfo = NULL;

	m_SkillInfoTable.SetPositionHead();
	while(pSInfo = m_SkillInfoTable.GetData())
	{
		delete pSInfo;
	}
	m_SkillInfoTable.RemoveAll();
/*	
	m_DummySkillInfoTable.SetPositionHead();
	while(p = m_DummySkillInfoTable.GetData())
	{
		delete p;
	}
	m_DummySkillInfoTable.RemoveAll();
*/	
	CSkillObject* pSObj = NULL;

	m_SkillObjectTable.SetPositionHead();
	while(pSObj = m_SkillObjectTable.GetData())
	{
		OBJECTMGR->AddGarbageObject((CObject*)pSObj);
		delete pSObj;
	}
	m_SkillObjectTable.RemoveAll();

	m_SkillAreaMgr.Release();

	PTRLISTSEARCHSTART(m_SkillChangeList,SKILL_CHANGE_INFO*,pInfo)
		delete pInfo;
	PTRLISTSEARCHEND
	m_SkillChangeList.RemoveAll();

	m_HwaKyungSkillTable.RemoveAll();
	m_GeukMaSkillTable.RemoveAll();

	//////////////////////////////////////////////////////////////////////////
	// 06. 06. 2차 전직 - 이영준
	// 무공 변환 추가
	SKILLOPTION* pSOpt = NULL;

	m_SkillOptionTable.SetPositionHead();
	while(pSOpt = m_SkillOptionTable.GetData())
	{
		delete pSOpt;
	}
	m_SkillOptionTable.RemoveAll();
	m_SkillOptionByItemTable.RemoveAll();
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	// 06. 06. 2차 전직 - 이영준
	// 은신/혜안
	SPECIAL_STATE_INFO* pStateInfo = NULL;

	m_SpecialStateInfoTable.SetPositionHead();
	while(pStateInfo = m_SpecialStateInfoTable.GetData())
	{
		delete pStateInfo;
	}
	m_SpecialStateInfoTable.RemoveAll();
	//////////////////////////////////////////////////////////////////////////

	////// 2007. 6. 28. CBH - 전문기술 확률 리스트 삭제 /////////////////
	JOB_SKILL_PROBABILITY_INFO* pJobSkillInfo = NULL;

	m_JobSkillProbabilityTable.SetPositionHead();
	while(pJobSkillInfo = m_JobSkillProbabilityTable.GetData())
	{
		SAFE_DELETE(pJobSkillInfo);		
	}
	m_JobSkillProbabilityTable.RemoveAll();
	////////////////////////////////////////////////////////////////////////

	// debug용
	m_nSkillUseCount = 0;
}

void CSkillManager::LoadSkillInfoList()
{
	CMHFile file;
#ifdef _FILE_BIN_
	file.Init("Resource/SkillList.bin","rb");
#else
	file.Init("Resource/SkillList.txt","rt");
#endif
	if(file.IsInited() == FALSE)
	{
		//ASSERTMSG(0,"SkillList를 로드하지 못했습니다.");
		return;
	}

	while(1)
	{
		if(file.IsEOF() != FALSE)
			break;

		CSkillInfo* pInfo = new CSkillInfo;
//		CSkillInfo* pDummyInfo = new CSkillInfo;
		pInfo->InitSkillInfo(&file);

		//	2005 크리스마스 이벤트 코드
		//////////////////////////////////////////////////////////////////////////
		//	스킬 잘 들어가나 체크중...
		WORD SkillIndex = pInfo->GetSkillIndex();
	
		//////////////////////////////////////////////////////////////////////////
		
//		pDummyInfo->InitDummySkillInfo(pInfo);

		ASSERT(m_SkillInfoTable.GetData(pInfo->GetSkillIndex()) == NULL);
		m_SkillInfoTable.Add(pInfo,pInfo->GetSkillIndex());
//		m_DummySkillInfoTable.Add(pDummyInfo,pInfo->GetSkillIndex());
	}

	file.Release();

	file.Init("Resource/SAT.bin","rb");
	if(file.IsInited() == FALSE)
	{
		MessageBox(0,"SAT.bin is not found",0,0);
		return;
	}

	int count = file.GetDword();
	for(int n=0;n<count;++n)
	{
		WORD skillIdx = file.GetWord();
		DWORD aniTimeMale = file.GetDword();
		DWORD aniTimeFemale = file.GetDword();

		CSkillInfo* pSkillInfo = GetSkillInfo(skillIdx);
		if(pSkillInfo == NULL)
			continue;

		pSkillInfo->SetSkillOperateAnimationTime(aniTimeMale,aniTimeFemale);
	}
	file.Release();

	LoadSkillTreeList();

	// 화경, 극마 리스트
	LoadJobSkillList();
	//////////////////////////////////////////////////////////////////////////
	// 06. 06. 2차 전직 - 이영준
	// 무공 변환 추가
	LoadSkillOptionList();
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	// 06. 06. 2차 전직 - 이영준
	// 은신/혜안
	LoadStateList();
	//////////////////////////////////////////////////////////////////////////
}

void CSkillManager::LoadSkillTreeList()
{
	CMHFile file;
	file.Init("Resource/SkillTree.bin","rb");

	if(file.IsInited() == FALSE)
	{
		return;
	}

	while(1)
	{
		if(file.IsEOF() != FALSE)
			break;

		WORD Array[5];

		file.GetString();
		file.GetWord(Array,5);
		
		for(int n=0;n<5;++n)
		{
			WORD Before = 0,After = 0;
			WORD Cur = Array[n];
			if(n != 0) Before = Array[n-1];
			if(n != 4) After = Array[n+1];
			
			CSkillInfo* pInfo = GetSkillInfo(Cur);
			if(pInfo == NULL)
				continue;

			pInfo->SetSkillTree(Before,After,Array);
		}
	}
	file.Release();
}


void CSkillManager::LoadJobSkillList()
{
	CMHFile file;
	file.Init("Resource/JobSkillList.bin","rb");

	if(file.IsInited() == FALSE)
	{
		return;
	}


	char buf[32];
	int Count = 0;
	DWORD SkillIdx = 0;

	while(1)
	{
		if(file.IsEOF() != FALSE)
			break;

		file.GetString(buf);

		if( strcmp( buf, "#HWAKUNG" ) == 0 )
		{
			Count = file.GetInt();

			for(int i=0; i<Count; ++i)
			{
				SkillIdx = file.GetDword();
				CSkillInfo* pInfo = m_SkillInfoTable.GetData( SkillIdx );
				if( !pInfo )		continue;

				m_HwaKyungSkillTable.Add( pInfo, SkillIdx );
			}
		}
		if( strcmp( buf, "#GEUKMA" ) == 0 )
		{
			Count = file.GetInt();

			for(int i=0; i<Count; ++i)
			{
				SkillIdx = file.GetDword();
				CSkillInfo* pInfo = m_SkillInfoTable.GetData( SkillIdx );
				if( !pInfo )		continue;

				m_GeukMaSkillTable.Add( pInfo, SkillIdx );
			}
		}
	}

	file.Release();
}


void CSkillManager::LoadSkillChangeInfoList()
{
	CMHFile file;
#ifdef _FILE_BIN_
	file.Init("Resource/SkillChangeList.bin","rb");
#else
	file.Init("Resource/SkillChangeList.txt","rt");
#endif
	if(file.IsInited() == FALSE)
	{
		ASSERTMSG(0,"SkillChangeList를 로드하지 못했습니다.");
		return;
	}

	while(1)
	{
		if(file.IsEOF() != FALSE)
			break;
		SKILL_CHANGE_INFO * pInfo = new SKILL_CHANGE_INFO;
		pInfo->wMugongIdx = file.GetWord();
		pInfo->wChangeRate = file.GetByte();
		pInfo->wTargetMugongIdx = file.GetWord();

		m_SkillChangeList.AddTail(pInfo);
	}
}

//////////////////////////////////////////////////////////////////////////
// 06. 06. 2차 전직 - 이영준
// 무공 변환 추가
void CSkillManager::LoadSkillOptionList()
{
	CMHFile file;
#ifdef _FILE_BIN_
	file.Init("Resource/SkillOptionList.bin","rb");
#else
	file.Init("Resource/SkillOptionList.txt","rt");
#endif
	if(file.IsInited() == FALSE)
	{
		ASSERTMSG(0,"SkillOptionList를 로드하지 못했습니다.");
		return;
	}

	while(1)
	{
		if(file.IsEOF() != FALSE)
			break;

		SKILLOPTION* pSOpt = new SKILLOPTION;

		memset(pSOpt, 0, sizeof(SKILLOPTION));

		pSOpt->Index		= file.GetWord();
		pSOpt->SkillKind	= file.GetWord();
		pSOpt->OptionKind	= file.GetWord();
		pSOpt->OptionGrade	= file.GetWord();
		pSOpt->ItemIndex	= file.GetWord();
		
		for(int i = 0; i < MAX_SKILLOPTION_COUNT; i++)
		{
			WORD Kind = file.GetWord();
			
			switch(Kind)
			{
			case eSkillOption_Range:
				pSOpt->Range = file.GetInt();
				break;
			
			case eSkillOption_ReduceNaeRyuk:
				pSOpt->ReduceNaeRyuk = file.GetFloat();
				break;
			
			case eSkillOption_PhyAtk:
				pSOpt->PhyAtk = file.GetFloat();
				break;
				
			case eSkillOption_BaseAtk:
				pSOpt->BaseAtk = file.GetFloat();
				break;
			
			case eSkillOption_AttAtk:
				pSOpt->AttAtk = file.GetFloat();
				break;

			case eSkillOption_Life:
				pSOpt->Life = file.GetInt();
				break;

			case eSkillOption_NaeRyuk:
				pSOpt->NaeRyuk = file.GetInt();
				break;

			case eSkillOption_Shield:
				pSOpt->Shield = file.GetInt();
				break;

			case eSkillOption_PhyDef:
				pSOpt->PhyDef = file.GetFloat();
				break;

			case eSkillOption_AttDef:
				pSOpt->AttDef = file.GetFloat();
				break;

			case eSkillOption_Duration:
				pSOpt->Duration = file.GetDword();
				break;

			case eSkillOption_None:
			default:
				file.GetWord();
				break;
			}
		}

		m_SkillOptionTable.Add(pSOpt, pSOpt->Index);
		m_SkillOptionByItemTable.Add(pSOpt, pSOpt->ItemIndex);
	}
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// 06. 06. 2차 전직 - 이영준
// 은신/혜안
void CSkillManager::LoadStateList()
{
	CMHFile file;
#ifdef _FILE_BIN_
	file.Init("Resource/StateInfo.bin","rb");
#else
	file.Init("Resource/StateInfo.txt","rt");
#endif
	if(file.IsInited() == FALSE)
	{
		ASSERTMSG(0,"StateList를 로드하지 못했습니다.");
		return;
	}

	while(1)
	{
		if(file.IsEOF() != FALSE)
			break;

		SPECIAL_STATE_INFO* pInfo = new SPECIAL_STATE_INFO;

		memset(pInfo, 0, sizeof(SPECIAL_STATE_INFO));

		pInfo->Index = file.GetWord();
		pInfo->IconIdx = file.GetWord();
		pInfo->TickTime = file.GetDword();
		pInfo->NaeRyuk = file.GetWord();
		pInfo->DelayTime = file.GetDword();
		pInfo->DurationTime = file.GetDword();

		m_SpecialStateInfoTable.Add(pInfo, pInfo->Index);
	}
}
//////////////////////////////////////////////////////////////////////////

CSkillInfo*	CSkillManager::GetSkillInfo(WORD SkillInfoIdx)
{
	return (CSkillInfo*)m_SkillInfoTable.GetData(SkillInfoIdx);
}
/*
CSkillInfo*	CSkillManager::GetDummySkillInfo(WORD SkillInfoIdx)
{
	return (CSkillInfo*)m_DummySkillInfoTable.GetData(SkillInfoIdx);
}
*/
WORD CSkillManager::GetSkillTooltipInfo(WORD SkillInfoIdx)
{
	SKILLINFO* skInfo = ((CSkillInfo*)m_SkillInfoTable.GetData(SkillInfoIdx))->GetSkillInfo();
	if(!skInfo)
		return 0;
	return skInfo->SkillTooltipIdx;
}
CSkillObject* CSkillManager::GetSkillObject(DWORD SkillObjectID)
{
	return (CSkillObject*)m_SkillObjectTable.GetData(SkillObjectID);
}
WORD CSkillManager::GetComboSkillIdx(CHero* pHero)
{
	WORD SkillNum = 0;
	int WeaponEquipType = pHero->GetWeaponEquipType();
	int CurComboNum = pHero->GetCurComboNum();
	if( CurComboNum == SKILL_COMBO_NUM || 
		CurComboNum == MAX_COMBO_NUM ||
		CurComboNum >= pHero->GetLevel() * 2)
	{
		CurComboNum = 0;
		pHero->SetCurComboNum(0);
	}

	// 현재 콤보+1 번의 콤보의 어택번호를 얻어와야 하지만
	// SkillNum = COMBO_???_MIN + 콤보번호 - 1 = COMBO_???_MIN + CurComboNum +1 -1
	// so... COMBO_???_MIN + CurComboNum 이다.
	switch(WeaponEquipType)
	{
	case WP_GUM:
		SkillNum = COMBO_GUM_MIN + CurComboNum;
		break;
	case WP_GWUN:
		SkillNum = COMBO_GWUN_MIN + CurComboNum;
		break;
	case WP_DO:
		SkillNum = COMBO_DO_MIN + CurComboNum;
		break;
	case WP_CHANG:
		SkillNum = COMBO_CHANG_MIN + CurComboNum;
		break;
	case WP_GUNG:
		SkillNum = COMBO_GUNG_MIN + CurComboNum;
		break;
	case WP_AMGI:
		SkillNum = COMBO_AMGI_MIN + CurComboNum;
		break;
	//	2005 크리스마스 이벤트 코드
	case WP_EVENT:
		SkillNum = COMBO_EVENT_MIN;
		break;
	// 2006 추석
	case WP_EVENT_HAMMER:
		SkillNum = COMBO_EVENT_HAMMER;
		break;
	}

	// !!!!!!!!!!! magi82 - 원래는 타이탄 콤보를 따로 만들어야하지만 지금은 급해서 일단 이렇게 임시로 씀 !!!!!!!!!!!!!!!!!!1
	if(pHero->InTitan())
		SkillNum += 10000;

	return SkillNum;
}

//////////////////////////////////////////////////////////////////////////
// 무공 정보를 퀵매니져에서 선택된 정보를 가져와
// 무공창에서 더블클릭으로 사용할시 단축창에서 사용되었던 무공이
// 사용되는 버그로 스킬 매니저가 마지막으로 사용한 스킬을 저장하도록 하여
// 사용하지 않는 함수
/*
WORD CSkillManager::GetMugongSkillIdx(CHero* pHero)
{	
	WORD SkillNum = 0;
#ifdef _TESTCLIENT_
	SkillNum = GAMERESRCMNGR->m_TestClientInfo.MugongIdx;
#else
	int abspos = QUICKMGR->GetSelectedQuickAbsPos();
	CQuickItem* pQuickItem;
	pQuickItem = GAMEIN->GetQuickDialog()->GetQuickItem(abspos);
	if(pQuickItem == NULL)
		return FALSE;
	SkillNum = pQuickItem->GetSrcIdx();
#endif
	return SkillNum;
}
*/
//////////////////////////////////////////////////////////////////////////

// 스킬을 사용
BOOL CSkillManager::ExcuteSkillSYN(CHero* pHero,CActionTarget* pTarget,BOOL bMugong)
{
/*
	WORD SkillNum;
	if(bMugong == FALSE)
		SkillNum = GetComboSkillIdx(pHero);
	else
		SkillNum = GetMugongSkillIdx(pHero);

	CSkillInfo* pSkillInfo = GetSkillInfo(SkillNum);
*/
	WORD SkillNum;
	CSkillInfo* pSkillInfo;

	if(bMugong == FALSE)
	{
		SkillNum = GetComboSkillIdx(pHero);
		pSkillInfo = GetSkillInfo(SkillNum);
	}
	else
		pSkillInfo = m_pSkillInfo;

	//ASSERT(pSkillInfo);
	if(pSkillInfo == NULL) 
		return FALSE;

	return ExcuteSkillSYN(pHero,pTarget,pSkillInfo);
}

BOOL CSkillManager::ExcuteSkillSYN(CHero* pHero,CActionTarget* pTarget,CSkillInfo* pSkillInfo)
{
	CActionTarget target;
	target.CopyFrom(pTarget);
	SKILLOPTION* pSkillOption = NULL;

	//ASSERT(pSkillInfo);
	if(pSkillInfo == NULL)
		return FALSE;
	
	int SkillLevel = 0;
	//if( pHero->InTitan() && pSkillInfo->GetSkillIndex() > SKILLNUM_TO_TITAN )
	//	SkillLevel = pHero->GetMugongLevel( pSkillInfo->GetSkillIndex() - SKILLNUM_TO_TITAN );
	//else
		SkillLevel = pHero->GetMugongLevel(pSkillInfo->GetSkillIndex());
	
	//SW070127 타이탄
	if( 0 == SkillLevel )
		return FALSE;

	WORD wSkillKind = pSkillInfo->GetSkillKind();

	// magi82 - Titan(070912) 타이탄 무공업데이트
	// 이제 타이탄 무기와 캐릭터의 무기는 별개이다.(무기가 서로 다르다고 해서 스킬이 안나가는게 아님)
	//if( (pHero->InTitan() == TRUE) && (CheckSkillKind(wSkillKind) == FALSE) )
	//{
	//	WORD weapon = pHero->GetWeaponEquipType();
	//	WORD titanWeapon = pHero->GetTitanWeaponEquipType();
	//	if(weapon != titanWeapon)
	//	{
	//		pHero->DisableAutoAttack(); //전문 스킬을 쓰면 무조건 자동어택 기능을 끈다.		
	//		CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(1644));
	//		return FALSE;
	//	}
	//}

	WORD SkillOptionIndex = pHero->GetSkillOptionIndex(pSkillInfo->GetSkillIndex());
	
	if(SkillOptionIndex)
		pSkillOption = m_SkillOptionTable.GetData(SkillOptionIndex);

	if(pSkillInfo->IsExcutableSkillState(pHero,SkillLevel,pSkillOption) == FALSE)
	{
		WORD wSkillKind = pSkillInfo->GetSkillKind();
		if( CheckSkillKind(wSkillKind) == FALSE )
		{
			pHero->SetCurComboNum(0);
		}	
		//"공격못해!"
		return FALSE;
	}
	
	// 진법 사용이면 진법 메니져 쪽으로 돌린다.
	if(pSkillInfo->GetSkillKind() == SKILLKIND_JINBUB)
	{
/*
				if( pTarget->GetTargetID() != 0 )
				{
					CObject* pTargetObj = OBJECTMGR->GetObject( pTarget->GetTargetID() );
					if( pTargetObj )
					{
		                if( !PARTYWAR->IsEnemy( (CPlayer*)pTargetObj ) )	return FALSE;
					}
				}
*/		

		// 진법Start번호와 진법Skill번호는 같다.
		TACTICMGR->HeroTacticStart(pSkillInfo->GetSkillIndex());
		return FALSE;
	}

	if(pSkillInfo->ComboTest(pHero) == FALSE)
	{
		return FALSE;
	}
	
	if(pSkillInfo->ConvertTargetToSkill(pHero,&target) == FALSE)
	{
		return FALSE;
	}

	// 2007. 7. 3. CBH - 전문스킬발동시 몬스터와의 관계 처리 함수 추가
	if(!IsJobSkill(pHero, pTarget, pSkillInfo))
	{
		return FALSE;
	}

	if(pSkillInfo->IsValidTarget(pHero, &target) == FALSE)
		return FALSE;

	// magi82(5) - Titan(071023) 타이탄 무공시 무기 체크할때 타겟 체크보다 뒤에 놓아야 타겟이 몬스터가 아닐때 채팅메세지가 뜨지않는다.
	if(CheckTitanWeapon(pHero, pSkillInfo->GetSkillInfo()) == FALSE)
	{
		return FALSE;
	}

//KES 040308

	if(PEACEWARMGR->IsPeaceMode(pHero) == TRUE)		//KES옮김
		PEACEWARMGR->ToggleHeroPeace_WarMode();
	
// RaMa 화경, 극마 체크
	if( pHero->GetStage() & eStage_Hwa )
	{
		if( m_GeukMaSkillTable.GetData( pSkillInfo->GetSkillIndex() ) )
		{
			CHATMGR->AddMsg( CTC_SYSMSG, CHATMGR->GetChatMsg(1144), CHATMGR->GetChatMsg(892) );
			return FALSE;
		}
	}
	else if( pHero->GetStage() & eStage_Geuk )
	{
		if( m_HwaKyungSkillTable.GetData( pSkillInfo->GetSkillIndex() ) )
		{
			CHATMGR->AddMsg( CTC_SYSMSG, CHATMGR->GetChatMsg(1144), CHATMGR->GetChatMsg(890) );
			return FALSE;
		}
	}

	
//
	VECTOR3* pTargetPos = target.GetTargetPosition();
	if(pTargetPos == NULL)
		return FALSE;
	
	if(pSkillInfo->IsInSkillRange(pHero,&target,pSkillOption) == TRUE)
	{	// 성공		
		MOVEMGR->HeroMoveStop();
		
		//자신이 타겟일 경우에는 보는 방향을 바꾸지 않는다.
		if( pSkillInfo->GetSkillInfo()->TargetKind != 1 )
			MOVEMGR->SetLookatPos(pHero,pTargetPos,0,gCurTime);
				
		pHero->SetMovingAction(NULL);		

		////////////////////////////////////////////////////////
		//06. 06 2차 전직 - 이영준
		//이펙트 생략(무초)
		//무초 상태에서 무초 가능 무공을 쓸때 시전연출 생략
		if( pHero->IsSkipSkill() == eSkipEffect_Start && pSkillInfo->CanSkipEffect() )
			return RealExcuteSkillSYN(pHero,&target,pSkillInfo);
		////////////////////////////////////////////////////////		

		if( pSkillInfo->GetSkillInfo()->EffectStart != 0 &&
			pSkillInfo->GetSkillInfo()->EffectStartTime != 0)	// 시전연출이 있을 경우
		{
			return HeroSkillStartEffect(pHero,&target,pSkillInfo);
		}
		else
		{
			return RealExcuteSkillSYN(pHero,&target,pSkillInfo);
		}
	}
	else
	{	// 실패
		pHero->SetCurComboNum(0);
		CAction MoveAction;
		if(pSkillInfo->IsMugong() == FALSE)
		{
			pSkillInfo = GetSkillInfo(GetComboSkillIdx(pHero));
		}
		MoveAction.InitSkillAction(pSkillInfo,&target);
		MOVEMGR->SetHeroActionMove(&target,&MoveAction);
		return FALSE;
	}
	

	return TRUE;
}

BOOL CSkillManager::ExcuteTacticSkillSYN(CHero* pHero,TACTIC_TOTALINFO* pTInfo,BYTE OperNum)
{
	CActionTarget target;
	target.InitActionTarget(&pTInfo->Pos,NULL);

	CSkillInfo* pSkillInfo = GetSkillInfo(pTInfo->TacticId);
	//ASSERT(pSkillInfo);
	if(pSkillInfo == NULL)
		return FALSE;

	return RealExcuteSkillSYN(pHero,&target,pSkillInfo);
}

void CSkillManager::GetMultiTargetList(CSkillInfo* pSkillInfo,CHero* pHero,CActionTarget* pTarget)
{
	WORD Radius = pSkillInfo->GetSkillInfo()->TargetRange;
	WORD AreaNum = pSkillInfo->GetSkillInfo()->TargetAreaIdx;
	if(AreaNum != 0)
	{
		CSkillArea* pSkillArea = GetSkillArea(pHero,pTarget,pSkillInfo);	// Area의 중심좌표까지 셋팅되어져 온다.
		pTarget->SetTargetObjectsInArea(pHero,pSkillArea);
	}
	else if(Radius != 0)
	{
		/// 06. 08. 자기중심범위형 스킬 버그 수정 - 이영준
		/// 자기 중심 범위일 경우에 스텟과 무공 변환에 의한 사정거리 효과가
		/// 무공 범위에 적용되어야 한다.
		if( pSkillInfo->GetSkillInfo()->TargetAreaPivot == 1 && pSkillInfo->GetSkillInfo()->TargetRange != 0 )
		{
			Radius += (WORD)HERO->GetAddAttackRange();

			WORD SkillOptionIndex = HERO->GetSkillOptionIndex(pSkillInfo->GetSkillIndex());
			SKILLOPTION* pSkillOption = NULL;

			if(SkillOptionIndex)
			{
				pSkillOption = m_SkillOptionTable.GetData(SkillOptionIndex);
			}

			if(pSkillOption)
			{
				Radius += pSkillOption->Range;
			}	
		}

		pTarget->SetTargetObjectsInRange(pSkillInfo->GetTargetAreaPivotPos(&pHero->GetCurPosition(),pTarget->GetTargetPosition()),Radius);
	}
}

BOOL CSkillManager::RealExcuteSkillSYN(CHero* pHero,CActionTarget* pTarget,CSkillInfo* pSkillInfo)
{
/*
	CSkillInfo* pDummyInfo = GetDummySkillInfo(pSkillInfo->GetSkillIndex());
	if(pDummyInfo->CheckOriginal(pSkillInfo) == FALSE)
	{
		MSGBASE msg;
		SetProtocol(&msg,MP_USERCONN,MP_USERCONN_CHEAT_USING);
		msg.dwObjectID = HEROID;
		NETWORK->Send(&msg,sizeof(msg));

//		MAINGAME->SetGameState(eGAMESTATE_END);
		
		return FALSE;
	}
*/
	SKILLOPTION* pSkillOption = NULL;

	if(pSkillInfo == NULL)
		return FALSE;
		
	int SkillLevel = pHero->GetMugongLevel(pSkillInfo->GetSkillIndex());

	WORD SkillOptionIndex = pHero->GetSkillOptionIndex(pSkillInfo->GetSkillIndex());
	
	if(SkillOptionIndex)
		pSkillOption = m_SkillOptionTable.GetData(SkillOptionIndex);

	if(pSkillInfo->IsExcutableSkillState(pHero,SkillLevel,pSkillOption) == FALSE)
	{
		pHero->SetCurComboNum(0);
		//"공격못해!"
		return FALSE;
	}

	// 2005 크리스마스 이벤트
	if( pHero->GetWeaponEquipType() == WP_EVENT && pSkillInfo->GetSkillIndex() == COMBO_EVENT_MIN )
	{
		//CItem* pItem = GAMEIN->GetInventoryDialog()->GetItemLike( EVENT_ITEM_SNOW );
		//SW061211 크리스마스이벤트
		CItem* pItem = GAMEIN->GetInventoryDialog()->GetPriorityItemForCristmasEvent();
		
		if( !pItem )
		{
		//	CHATMGR->AddMsg( CTC_SYSMSG, CHATMGR->GetChatMsg(583) );
			return	FALSE;
		}

//		GAMEIN->GetInventoryDialog()->UseItem( pItem );
		
		// 더블 클릭으로 사용되어지는 것때문에 직접 전송
		MSG_ITEM_USE_SYN msg;

		msg.Category = MP_ITEM;
		msg.Protocol = MP_ITEM_USE_SYN;
		msg.dwObjectID = HEROID;
		msg.wItemIdx = pItem->GetItemIdx();
		msg.TargetPos = pItem->GetPosition();

		NETWORK->Send(&msg,sizeof(msg));

		// debug용
		ITEMMGR->m_nItemUseCount++;
	}

	// 진법시 내력부족
	if(pSkillInfo->GetSkillKind() == SKILLKIND_JINBUB && pHero->GetNaeRyuk() < pSkillInfo->GetNeedNaeRyuk(1))
	{
		CHATMGR->AddMsg( CTC_SYSMSG, CHATMGR->GetChatMsg(401) );		
		return FALSE;
	}

	//공격하면 pk지속시간을 연장한다.
	if( HERO->IsPKMode() )
	{
		CObject* pObject = OBJECTMGR->GetObject(pTarget->GetTargetID());
		if( pObject )
		if( pObject->GetObjectKind() == eObjectKind_Player )
		{
			int SkillLevel = pHero->GetMugongLevel(pSkillInfo->GetSkillIndex());
			if( pSkillInfo->GetFirstPhyAttack( SkillLevel ) ||
				pSkillInfo->GetFirstAttAttack( SkillLevel ) )
			{
				PKMGR->SetPKStartTimeReset();
			}
		}
	}

	HERO->SetNextAction(NULL);

	if(pSkillInfo->IsMultipleTargetSkill() == TRUE)
	{
		pTarget->ConvertMainTargetToPosition(pHero,pSkillInfo->GetSkillRange());
		GetMultiTargetList(pSkillInfo,pHero,pTarget);
	}

	CSkillObject* pSObj = CreateTempSkillObject(pSkillInfo,pHero,pTarget);
	if(pSObj == NULL)
		return FALSE;
	
	pSkillInfo->SendMsgToServer(pHero,pTarget);
	
	pHero->SetSkillCoolTime(pSkillInfo->GetSkillInfo()->DelayTime);

	if(pHero->IsSkipSkill())
	{
		pHero->GetDelayGroup()->AddDelay(
			CDelayGroup::eDK_Skill,pSkillInfo->GetSkillIndex(),
			pSkillInfo->GetSkillInfo()->DelayTime - pSkillInfo->GetSkillOperateAnimationTime( 1 ));
	}
	else
	{
		pHero->GetDelayGroup()->AddDelay(
			CDelayGroup::eDK_Skill,pSkillInfo->GetSkillIndex(),
			pSkillInfo->GetSkillInfo()->DelayTime);
	}
	//여기서 나머지 스킬도 빨갛게 만들어주자.
	SKILLDELAYMGR->AddSkillDelay( pSkillInfo->GetSkillIndex() );


	pHero->SetCurComboNum(pSkillInfo->GetSkillInfo()->ComboNum);

	m_pSkillInfo = NULL;

#ifdef _TESTCLIENT_
	static IDDDD = 0;
	MSG_SKILLOBJECT_ADD msg;
	SKILLOBJECT_INFO info;
	memcpy(&info,pSObj->GetSkillObjectInfo(),sizeof(SKILLOBJECT_INFO));
	info.SkillObjectIdx = SKILLOBJECT_ID_START+IDDDD++;
	info.StartTime = gCurTime;
	info.SkillLevel = 5;
	msg.InitMsg(&info,TRUE);
	CTargetListIterator iter(&msg.TargetList);
	pTarget->SetPositionFirstTargetObject();
	CBattle* pBattle = BATTLESYSTEM->GetBattle(HERO);
	while(CObject* pObject = pTarget->GetNextTargetObject())
	{
		RESULTINFO dinfo;
		dinfo.Clear();
		if(pBattle->IsEnemy(pHero,pObject) == TRUE)
		{
			dinfo.bCritical = 1;//rand()%5 ? FALSE : TRUE;
			dinfo.RealDamage = rand() % 50;
			dinfo.ShieldDamage = rand() % 30;
			dinfo.CounterDamage = 0;
			dinfo.StunTime = 1000;//(rand() % 30 == 0) ? 2000 : 0;
			iter.AddTargetWithResultInfo(pObject->GetID(),SKILLRESULTKIND_NEGATIVE,&dinfo);
		}
		else
		{
			iter.AddTargetWithResultInfo(pObject->GetID(),SKILLRESULTKIND_POSITIVE,&dinfo);
		}

		
	}

	iter.Release();

	NetworkMsgParse(MP_SKILL_SKILLOBJECT_ADD,&msg);
#endif

	//SW05810 평화모드 자동전환 작업
	PEACEWARMGR->SetCheckTime(gCurTime);

	return TRUE;
}

// 다음 콤보 스킬을 지정
void CSkillManager::SetNextComboSkill(CHero* pHero,CActionTarget* pTarget,BOOL bMugong)
{
	CSkillInfo* pNextSkill;
	WORD NextSkillIdx;
/*
	if(bMugong)
		NextSkillIdx = GetMugongSkillIdx(pHero);
	else
		NextSkillIdx = GetComboSkillIdx(pHero);
	pNextSkill = GetSkillInfo(NextSkillIdx);
*/ //GetMugongSkillIdx() 함수를 사용하지 않게 되어 수정
	if(bMugong)
		pNextSkill = m_pSkillInfo;
	else
	{
		NextSkillIdx = GetComboSkillIdx(pHero);
		pNextSkill = GetSkillInfo(NextSkillIdx);
	}
	
	if(pNextSkill == NULL)
		return;
	CAction act;
	act.InitSkillAction(pNextSkill,pTarget);
	pHero->SetNextAction(&act);
}

// 이벤트 핸들 함수들
BOOL CSkillManager::OnSkillCommand(CHero* pHero,CActionTarget* pTarget,BOOL bMugong)
{
	//////////////////////////////////////////////////////////////////////////
	// 죽은 사람은 공격할수 없음
	if(pTarget->GetTargetID() != 0)
	{
		CObject* pTObj = OBJECTMGR->GetObject(pTarget->GetTargetID());
		if(pTObj == NULL)
			return FALSE;
		if(pTObj->IsDied() == TRUE)
			return FALSE;

		if( pHero->GetWeaponEquipType() != WP_EVENT )
		{
			if( pTObj->GetObjectKind() & eObjectKind_Monster )
			{
				CMonster* pMonster = (CMonster*)pTObj;
				if(	pMonster->GetMonsterKind() == EVENT_FIELDBOSS_SNOWMAN_SM || pMonster->GetMonsterKind() == EVENT_FIELDSUB_SNOWMAN_SM
					|| pMonster->GetMonsterKind() == EVENT_FIELDBOSS_SNOWMAN_MD || pMonster->GetMonsterKind() == EVENT_FIELDSUB_SNOWMAN_MD
					|| pMonster->GetMonsterKind() == EVENT_FIELDBOSS_SNOWMAN_LG || pMonster->GetMonsterKind() == EVENT_FIELDSUB_SNOWMAN_LG
/*					|| pMonster->GetMonsterKind() == EVENT_SNOWMAN_SM
					|| pMonster->GetMonsterKind() == EVENT_SNOWMAN_MD
					|| pMonster->GetMonsterKind() == EVENT_SNOWMAN_LG*/ )
					return FALSE;
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////
	

	// Guild Tournament나 공성전에서 Observer이면 사용불가
	CBattle* pBattle = BATTLESYSTEM->GetBattle( HERO );
	if( pBattle && pBattle->GetBattleKind() == eBATTLE_KIND_GTOURNAMENT ||
		pBattle && pBattle->GetBattleKind() == eBATTLE_KIND_SIEGEWAR )
	{
		if( pHero->GetBattleTeam() == 2 )
			return FALSE;	
	}

	// 2005 크리스마스 이벤트 코드
	// 이벤트 무기 장착시 눈덩이가 없으면 공격불가 내공 사용 불가
	if( pHero->GetWeaponEquipType() == WP_EVENT )
	{
		if( bMugong )
		{
			CHATMGR->AddMsg( CTC_SYSMSG, CHATMGR->GetChatMsg(586) );	
			return FALSE;
		}
		else
		{
			//CItem* pItem = GAMEIN->GetInventoryDialog()->GetItemLike( EVENT_ITEM_SNOW );
			//SW061211 크리스마스이벤트
			CItem* pItem = GAMEIN->GetInventoryDialog()->GetPriorityItemForCristmasEvent();

			if( !pItem )
			{
				CHATMGR->AddMsg( CTC_SYSMSG, CHATMGR->GetChatMsg(583) );
				return	FALSE;
			}
		}
	}
	else if( pHero->GetWeaponEquipType() == WP_EVENT_HAMMER )
	{
		if( bMugong )
		{
			CHATMGR->AddMsg( CTC_SYSMSG, CHATMGR->GetChatMsg(798) );	
			return FALSE;
		}
	}

	if( pHero->GetState() == eObjectState_SkillSyn ||
		pHero->GetState() == eObjectState_SkillUsing)
	{
		// 2007. 7. 6. CBH - 전문스킬은 자동 공격을 막아야한다.
		CObject* pObject = OBJECTMGR->GetObject(pTarget->GetTargetID());
		if(pObject == NULL) //예외처리
		{
			return FALSE;
		}

		if( GetObjectKindGroup(pObject->GetObjectKind()) == eOBJECTKINDGROUP_JOB )
		{
			pHero->SetStage(eObjectState_None);
			return FALSE;
		}
		else
		{
			SetNextComboSkill(pHero,pTarget,bMugong);
		}		
	}
	else
		ExcuteSkillSYN(pHero,pTarget,bMugong);		//return FALSE 처리....없다.. 괜찮을까?
	return TRUE;
}
DWORD GetComboDelayTime(WORD WeaponKind)
{
	DWORD time = 0;
	ySWITCH(WeaponKind)
		yCASE(WP_GUM)	time = 120;
		yCASE(WP_GWUN)	time = 150;
		yCASE(WP_DO)	time = 150;
		yCASE(WP_CHANG)	time = 150;
		yCASE(WP_GUNG)	time = 50;
		yCASE(WP_AMGI)	time = 80;
	yENDSWITCH
	return time;
}
void CSkillManager::OnComboTurningPoint(CHero* pHero)
{
	if(pHero->GetNextAction()->HasAction())
	{
		if(pHero->GetNextAction()->GetActionKind() != eActionKind_Skill)
			pHero->SetCurComboNum(0);

		pHero->GetNextAction()->ExcuteAction(pHero);
		pHero->GetNextAction()->Clear();
	}
	else
	{		
		if(pHero->IsAutoAttacking())
		{
			if(pHero->GetCurComboNum() < 2)	// 자동공격은 콤보 2까지만	12/3일 회의 결과 3에서 2로 바뀜
			{
				if(SKILLMGR->OnSkillCommand(pHero,pHero->GetAutoAttackTarget(),FALSE) == FALSE)
					pHero->DisableAutoAttack();
			}
			else
			{
				OBJECTSTATEMGR->StartObjectState(pHero,eObjectState_SkillDelay);
				OBJECTSTATEMGR->EndObjectState(pHero,eObjectState_SkillDelay,GetComboDelayTime(pHero->GetWeaponEquipType()));
				pHero->SetCurComboNum(0);
			}
		}
	}
}
void CSkillManager::OnExcuteSkillNACKed(SKILLOBJECT_INFO* pInfo)
{
	//ASSERT(0);
}

// SkillObject 등록 및 해제
void CSkillManager::DoCreateSkillObject(CSkillObject* pSObj,SKILLOBJECT_INFO* pSOInfo,CTargetList* pTList)
{
	pSObj->Init(pSOInfo,pTList);
	
	CSkillObject* pPreObj = m_SkillObjectTable.GetData(pSObj->GetID());
	//ASSERT(pPreObj == NULL);	
	if(pPreObj != NULL)
	{
		ReleaseSkillObject(pPreObj);
	}

	m_SkillObjectTable.Add(pSObj,pSObj->GetID());
	OBJECTMGR->AddSkillObject(pSObj);
}

CSkillObject* CSkillManager::CreateSkillObject(MSG_SKILLOBJECT_ADD* pSkillObjectAddInfo)
{
	CObject* pOperator = OBJECTMGR->GetObject(pSkillObjectAddInfo->SkillObjectInfo.Operator);

	////////////////////////////////////////////////////////////////////
	/// 06. 08. 2차 보스 - 이영준
	/// 일부 스킬 사용시 방향을 더 틀기 위해
	/// 미리 스킬 정보를 가져와야 한다
	WORD SkillIdx = pSkillObjectAddInfo->SkillObjectInfo.SkillIdx;
	CSkillInfo* pSkillInfo = GetSkillInfo(SkillIdx);
	CSkillObject* pSObj = pSkillInfo->GetSkillObject();
	////////////////////////////////////////////////////////////////////

	if(pOperator == NULL)
	{
		OBJECTACTIONMGR->ApplyTargetList(pOperator,&pSkillObjectAddInfo->TargetList,eDamageKind_Front);
		return NULL;
	}
	if(pOperator->GetObjectKind() & eObjectKind_Monster)
	{
		VECTOR3 MainTargetPos;
		GetMainTargetPos(&pSkillObjectAddInfo->SkillObjectInfo.MainTarget,&MainTargetPos,NULL);
		/////////////////////////////////////////////////////////////////////////////////////
		/// 06. 08. 2차 보스 - 이영준
		/// 기존 SetLookatPos 함수 마지막 인자에 추가 회전각을 추가했다.
		MOVEMGR->SetLookatPos(pOperator,&MainTargetPos,0,gCurTime, pSkillInfo->GetAddDegree());
		/////////////////////////////////////////////////////////////////////////////////////
	}		
	else
		MOVEMGR->SetAngle(pOperator,DIRTODEG(pSkillObjectAddInfo->SkillObjectInfo.Direction),0);


	DoCreateSkillObject(pSObj,&pSkillObjectAddInfo->SkillObjectInfo,
						&pSkillObjectAddInfo->TargetList);

	return pSObj;
}


CSkillObject* CSkillManager::CreateTempSkillObject(CSkillInfo* pSkillInfo,CHero* pHero,CActionTarget* pTarget)
{
	///////////////////////////////////////////////////////////////////////////
	// 06. 04. 스킬 버그 수정 - 이영준
	// 임시객체가 남아있다면 스킬을 생성하지 않는다
	CSkillObject* pTempObj = GetSkillObject(TEMP_SKILLOBJECT_ID);
	if(pTempObj)
	{
//		ASSERT(0);
		return NULL;
	}
	///////////////////////////////////////////////////////////////////////////

	CSkillObject* pSObj = pSkillInfo->GetSkillObject();
	
	SKILLOBJECT_INFO sinfo;
	sinfo.SkillIdx = pSkillInfo->GetSkillIndex();
	sinfo.SkillObjectIdx = TEMP_SKILLOBJECT_ID;
	sinfo.Direction = DEGTODIR(pHero->GetAngleDeg());
	pTarget->GetMainTarget(&sinfo.MainTarget);
	sinfo.Operator = pHero->GetID();
	VECTOR3* pHeroPos = &pHero->GetCurPosition();
	VECTOR3* pTargetPos = pTarget->GetTargetPosition();
	if(pTargetPos == NULL)
		return NULL;
	sinfo.Pos = *(pSkillInfo->GetTargetAreaPivotPos(pHeroPos,pTargetPos));
	sinfo.StartTime = gCurTime;

	CTargetList tlist;
	pTarget->GetTargetArray(&tlist);

	DoCreateSkillObject(pSObj,&sinfo,&tlist);

	// for debug
	
	return pSObj;
}

void CSkillManager::ChangeTempObjToRealObj(MSG_SKILLOBJECT_ADD* pSkillObjectAddInfo)
{
	CSkillObject* pTempObj = GetSkillObject(TEMP_SKILLOBJECT_ID);
//	//ASSERT(pTempObj);

	if(pTempObj == NULL)
	{
		return;
	}

	m_SkillObjectTable.Remove(pTempObj->GetID());
	OBJECTMGR->AddGarbageObject(pTempObj);
	
	DoCreateSkillObject(pTempObj,&pSkillObjectAddInfo->SkillObjectInfo,
							&pSkillObjectAddInfo->TargetList);

	// debug용
	m_nSkillUseCount--;
}

void CSkillManager::ReleaseSkillObject(CSkillObject* pSkillObject)
{
	CSkillInfo* pSInfo = GetSkillInfo(pSkillObject->GetSkillIdx());
	if(pSInfo == NULL)
	{
		char temp[256];
		sprintf(temp,"skillkind:%d  SkillName:%s  oper:%d",
			pSkillObject->GetSkillIdx(),
			pSkillObject->GetSkillInfo()->GetSkillName(),
			pSkillObject->GetOperator() ? pSkillObject->GetOperator()->GetID() : 0);
		ASSERTMSG(0,temp);
		return;
	}
	//ASSERT(pSInfo);

	m_SkillObjectTable.Remove(pSkillObject->GetID());
	OBJECTMGR->AddGarbageObject(pSkillObject);
	
	pSkillObject->ReleaseSkillObject();
	pSInfo->ReleaseSkillObject(pSkillObject,0);
	
}
void CSkillManager::OnReceiveSkillObjectAdd(MSG_SKILLOBJECT_ADD* pmsg)
{
	//////////////////////////////////////////////////////////////////
	// 06. 04. 스킬 버그 수정 - 이영준
	// 자기가 쓴 스킬이라도 
	// 스킬 최초 생성시에는 ChangeTempObjToRealObj를 호출하지만
	// 그뒤 그리드 이동등의 이유로 Add만 되는 경우에는
	// CreateSkillObject를 호출해서 생성해야한다.
    //
	//if(pmsg->SkillObjectInfo.Operator == HEROID)
	if(pmsg->SkillObjectInfo.Operator == HEROID && pmsg->bCreate)
		ChangeTempObjToRealObj(pmsg);
	else
		CreateSkillObject(pmsg);
}
void CSkillManager::OnReceiveSkillObjectRemove(MSG_DWORD* pmsg)
{
	CSkillObject* pSObj = GetSkillObject(pmsg->dwData);
	if(pSObj == NULL)
	{
		//ASSERTMSG(0,"SkillObject Remove Failed");
		return;
	}
	ReleaseSkillObject(pSObj);
}

void CSkillManager::OnReceiveSkillStartNack()
{
	HERO->SetCurComboNum(0);
	CSkillObject* pSObj = GetSkillObject(TEMP_SKILLOBJECT_ID);
	//ASSERT(pSObj);
	if(pSObj == NULL)
		return;

	if( HERO == pSObj->GetOperator() )
	{
		m_nSkillUseCount--;
	}

	pSObj->SetMissed();
	ReleaseSkillObject(pSObj);

}

void CSkillManager::OnReceiveSkillSingleResult(MSG_SKILL_SINGLE_RESULT* pmsg)
{
	CSkillObject* pSObj = GetSkillObject(pmsg->SkillObjectID);
	if(pSObj == NULL)
	{
		OBJECTACTIONMGR->ApplyTargetList(NULL,&pmsg->TargetList,eDamageKind_ContinueDamage);
		return;
	}

	pSObj->OnReceiveSkillSingleResult(pmsg);
}

void CSkillManager::OnReceiveSkillObjectTargetListAdd(DWORD SkillObjectID,DWORD AddedObjectID,BYTE bTargetKind)
{
	CSkillObject* pSObj = GetSkillObject(SkillObjectID);
	//ASSERT(pSObj);
	if(pSObj == NULL)
	{
		//ASSERTMSG(0,"SkillObject Target Add Failed(No SObj)");
		return;
	}
	CObject* pObject = OBJECTMGR->GetObject(AddedObjectID);
	if(pObject == NULL)
	{
		//ASSERTMSG(0,"SkillObject Target Add Failed(No Target)");
		return;
	}

	pSObj->AddTargetObject(pObject,bTargetKind);
}
void CSkillManager::OnReceiveSkillObjectTargetListRemove(DWORD SkillObjectID,DWORD RemovedObjectID)
{
	CSkillObject* pSObj = GetSkillObject(SkillObjectID);
	//ASSERT(pSObj);
	if(pSObj == NULL)
	{		
		//ASSERTMSG(0,"SkillObject Target Remove Failed");
		return;
	}

	pSObj->RemoveTargetObject(RemovedObjectID);
}

void CSkillManager::OnReceiveSkillStartEffect(DWORD SkillOperator,DWORD SkillIdx)
{
	CObject* pOperator = OBJECTMGR->GetObject(SkillOperator);
	if(pOperator == NULL)
	{
		//ASSERTMSG(0,"SkillObject StartEffect Failed(No Operator)");
		return;
	}
	
	CSkillInfo* pSkillInfo = GetSkillInfo((WORD)SkillIdx);
	if(pSkillInfo == NULL)
	{		
		//ASSERTMSG(0,"SkillObject StartEffect Failed(No SkillInfo)");
		return;
	}

	SkillStartEffect(pOperator,pSkillInfo);
}

void CSkillManager::OnReceiveSkillObjectOperate(MSG_SKILL_OPERATE* pmsg)
{
	CSkillObject* pSObj = GetSkillObject(pmsg->SkillObjectID);
	//ASSERT(pSObj);
	if(pSObj == NULL)
	{		
		//ASSERTMSG(0,"SkillObject Operate Failed(No SObj)");
		return;
	}

	CObject* pReqObj = OBJECTMGR->GetObject(pmsg->RequestorID);
	pSObj->Operate(pReqObj,&pmsg->MainTarget,&pmsg->TargetList);

}

void CSkillManager::NetworkMsgParse(BYTE Protocol,void* pMsg)
{
	printf("[%d] %d // %d\n", ((MSGBASE*)pMsg)->CheckSum, Protocol, ((MSGBASE*)pMsg)->dwObjectID);

	switch(Protocol)
	{
	case MP_SKILL_SKILLOBJECT_ADD:
		{
			MSG_SKILLOBJECT_ADD* pmsg = (MSG_SKILLOBJECT_ADD*)pMsg;
			printf("%d\n", pmsg->SkillObjectInfo.SkillIdx);
			OnReceiveSkillObjectAdd(pmsg);
		}
		break;
	case MP_SKILL_SKILLOBJECT_REMOVE:
		{
			MSG_DWORD* pmsg = (MSG_DWORD*)pMsg;
			OnReceiveSkillObjectRemove(pmsg);
		}
		break;
	case MP_SKILL_START_NACK:
		{
			MSG_BYTE* pmsg = (MSG_BYTE*)pMsg;
			OnReceiveSkillStartNack();
		}
		break;
	case MP_SKILL_SKILL_SINGLE_RESULT:
		{
			MSG_SKILL_SINGLE_RESULT* pmsg = (MSG_SKILL_SINGLE_RESULT*)pMsg;
			OnReceiveSkillSingleResult(pmsg);
		}
		break;

		// TargetList Update
	case MP_SKILL_ADDOBJECT_TO_SKILLOBJECTAREA_ACK:
		{
			MSG_DWORD2BYTE* pmsg = (MSG_DWORD2BYTE*)pMsg;
			OnReceiveSkillObjectTargetListAdd(pmsg->dwData1,pmsg->dwData2,pmsg->bData);
		}
		break;
	case MP_SKILL_REMOVEOBJECT_FROM_SKILLOBJECTAREA_ACK:
		{
			MSG_DWORD2* pmsg = (MSG_DWORD2*)pMsg;
			OnReceiveSkillObjectTargetListRemove(pmsg->dwData1,pmsg->dwData2);
		}
		break;

	case MP_SKILL_STARTEFFECT:
		{
			MSG_DWORD2* pmsg = (MSG_DWORD2*)pMsg;
			OnReceiveSkillStartEffect(pmsg->dwData1,pmsg->dwData2);
		}
		break;

	case MP_SKILL_OPERATE_ACK:
		{
			MSG_SKILL_OPERATE* pmsg = (MSG_SKILL_OPERATE*)pMsg;
			OnReceiveSkillObjectOperate(pmsg);
		}
		break;
		
	case MP_SKILL_DELAY_NOTIFY:
		{
			MSG_DWORD2* pmsg = (MSG_DWORD2*)pMsg;

			SKILLDELAYMGR->ContinueSkillDelay( pmsg->dwData1, pmsg->dwData2 );
		}
		break;
	case MP_SKILL_JOB_NACK:
		{
			CHATMGR->AddMsg(CTC_SYSMSG, "스킬 실패.");
		}
		break;
	}
}

CSkillArea* CSkillManager::GetSkillArea(CObject* pObject, CActionTarget* pTarget, CSkillInfo* pSkillInfo)
{
	CSkillArea* pArea = GetSkillArea(pObject->GetDirectionIndex(),pSkillInfo->GetSkillAreaIndex());
	
	// Area의 중심좌표까지 셋팅되어져 온다.
	VECTOR3* pPos;
	pPos = pSkillInfo->GetTargetAreaPivotPos(&pObject->GetCurPosition(),pTarget->GetTargetPosition());
	pArea->SetCenterPos(pPos);

	return pArea;	
}

CSkillArea* CSkillManager::GetSkillArea(DIRINDEX dir,WORD SkillAreaIndex)
{
	CSkillArea* pArea = m_SkillAreaMgr.GetSkillArea(SkillAreaIndex,dir);
	
	return pArea;
}

void CSkillManager::ReleaseSkillArea(CSkillArea* pSkillArea)
{
	m_SkillAreaMgr.ReleaseSkillArea(pSkillArea);
}

void CSkillManager::UpdateSkillObjectTargetList(CObject* pObject)
{
	CSkillObject* pSObj;
	DWORD rtCode;
	m_SkillObjectTable.SetPositionHead();
	while(pSObj = m_SkillObjectTable.GetData())
	{
		rtCode = pSObj->Update();		// 자기가 쓰고 있는 스킬들에 대한 몬스터의 업데이트
		pSObj->UpdateTargetList(pObject);	// 주인공 업데이트
#ifdef _TESTCLIENT_
		if(rtCode == SO_DESTROYOBJECT)
		{
			m_SkillObjectTable.Remove(pSObj->GetID());
			ReleaseSkillObject(pSObj);
		}
#endif
	}
}

void CSkillManager::RemoveTarget(CObject* pObject,BYTE bTargetKind)
{
	CSkillObject* pSObj;
	m_SkillObjectTable.SetPositionHead();
	while(pSObj = m_SkillObjectTable.GetData())
	{
		pSObj->RemoveTarget(pObject,bTargetKind);
	}
}

void CSkillManager::SkillStartEffect(CObject* pObject,CSkillInfo* pSkillInfo)
{
	TARGETSET set;
	set.pTarget = pObject;
	DWORD flag = 0;
	if(pObject->GetID() == HEROID)
		flag |= EFFECT_FLAG_HEROATTACK;
	EFFECTMGR->StartEffectProcess(pSkillInfo->GetSkillInfo()->EffectStart,pObject,
								&set,1,pObject->GetID(),flag);
}

BOOL CSkillManager::HeroSkillStartEffect(CHero* pHero,CActionTarget* pTarget,CSkillInfo* pSkillInfo)
{
	SkillStartEffect(pHero,pSkillInfo);
	OBJECTSTATEMGR->StartObjectState(pHero,eObjectState_SkillStart);
	OBJECTSTATEMGR->EndObjectState(pHero,eObjectState_SkillStart,pSkillInfo->GetSkillInfo()->EffectStartTime);
	CAction act;
	act.InitSkillActionRealExecute(pSkillInfo,pTarget);
	pHero->SetSkillStartAction(&act);

	// 서버에 메세지 보낸다.
	MSG_DWORD2 msg;
	msg.Category = MP_SKILL;
	msg.Protocol = MP_SKILL_STARTEFFECT;
	msg.dwData1 = pHero->GetID();
	msg.dwData2 = pSkillInfo->GetSkillIndex();
	NETWORK->Send(&msg,sizeof(msg));

	return TRUE;
}

void CSkillManager::MakeSAT()
{
	FILE* fp = fopen("SAT.txt","w");
	fprintf(fp,"%d\n",m_SkillInfoTable.GetDataNum());
	
	CEngineObject man,woman;
	man.Init("man.chx",NULL,eEngineObjectType_Character);
	woman.Init("woman.chx",NULL,eEngineObjectType_Character);
	CSkillInfo* pSkillInfo;
	m_SkillInfoTable.SetPositionHead();
	while(pSkillInfo = m_SkillInfoTable.GetData())
	{
		DWORD StateEndTimeMan = 0;
		DWORD StateEndTimeWoman = 0;
		BOOL bBinding = pSkillInfo->GetSkillInfo()->BindOperator != 0;
		if(!bBinding)
		{
			WORD effectuse = pSkillInfo->GetSkillInfo()->EffectUse;
			StateEndTimeMan = EFFECTMGR->GetOperatorAnimatioEndTime(effectuse,eEffectForMan,&man);
			StateEndTimeWoman = EFFECTMGR->GetOperatorAnimatioEndTime(effectuse,eEffectForWoman,&woman);
			if(StateEndTimeMan == 0)	StateEndTimeMan = 500;
			if(StateEndTimeWoman == 0)	StateEndTimeWoman = 500;			
		}

		fprintf(fp,"%d\t%d\t%d\n",pSkillInfo->GetSkillIndex(),StateEndTimeMan,StateEndTimeWoman);
	}

	man.Release();
	woman.Release();

	fclose(fp);
}

SKILL_CHANGE_INFO * CSkillManager::GetSkillChangeInfo(WORD wMugongIdx)
{
	PTRLISTSEARCHSTART(m_SkillChangeList, SKILL_CHANGE_INFO *, pInfo)
		if(pInfo->wMugongIdx == wMugongIdx)
			return pInfo;
	PTRLISTSEARCHEND
	return NULL;
}

BOOL CSkillManager::IsDeadlyMugong(WORD wMugongIdx)
{
	PTRLISTSEARCHSTART(m_SkillChangeList, SKILL_CHANGE_INFO *, pInfo)
		if(pInfo->wTargetMugongIdx == wMugongIdx)
			return TRUE;
	PTRLISTSEARCHEND
	return FALSE;
}

///////////////////////////////////////////////////////////////////
// 06. 04. 스킬 버그 수정 - 이영준
// 임시스킬객체를 강제로 지워주는 함수
void CSkillManager::DeleteTempSkill()
{
	CSkillObject* pSObj = GetSkillObject(TEMP_SKILLOBJECT_ID);

	ReleaseSkillObject(pSObj);
}
///////////////////////////////////////////////////////////////////

////////// 2007. 6. 28. CBH - 전문기술 확률 관련 함수 추가 ////////////////////
BOOL CSkillManager::LoadJobSkillProbability()
{
	CMHFile file;
	if(!file.Init("Resource/jobskill.bin","rb"))
	{
		return FALSE;
	}

	JOB_SKILL_PROBABILITY_INFO* pJobSkillInfo;
	while(!file.IsEOF())
	{
		pJobSkillInfo = new JOB_SKILL_PROBABILITY_INFO;
		memset( pJobSkillInfo, 0, sizeof(JOB_SKILL_PROBABILITY_INFO) );

		pJobSkillInfo->wSkillLevel = file.GetWord();
		file.GetWord(pJobSkillInfo->ProbabilityArray, MAX_JOBMOB_NUM);

		ASSERT(!m_JobSkillProbabilityTable.GetData(pJobSkillInfo->wSkillLevel));

		m_JobSkillProbabilityTable.Add(pJobSkillInfo, pJobSkillInfo->wSkillLevel);
		pJobSkillInfo = NULL;
	}
	file.Release();

	return TRUE;
}

BOOL CSkillManager::IsJobSkill(CHero* pHero,CActionTarget* pTarget, CSkillInfo* pSkillInfo)
{	
	// 타겟이 전문기술 오브젝트면 일반 스킬 막는다	
	// 전문기술 레벨보다 오브젝트의 레벨이 높으면 막는다. (메세지 처리)
	// 타겟이 일반 몹이고 전문스킬 시전시 시전 못하게 막는다.
	CObject* pObject = OBJECTMGR->GetObject(pTarget->GetTargetID());

	if(pObject == NULL)
	{
		return FALSE;
	}
	
	WORD wSkillKind = pSkillInfo->GetSkillKind();
	//타겟이 플레이어 일때 무공일때 처리
	if( pObject->GetObjectKind() == eObjectKind_Player )		
	{
		if(CheckSkillKind(wSkillKind) == TRUE)
		{
			CHATMGR->AddMsg(CTC_SYSMSG, "ΝА㎌�胥Aㄳ�鋤弔뻘甸� .");
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}

	//성문에서 뻑나는 부분 처리
	if(pObject->GetObjectKind() == eObjectKind_MapObject)
	{
		return TRUE;
	}

	WORD wObjectKind = pObject->GetObjectKind();	

	if(CheckSkillKind(wSkillKind) == TRUE)	
	{
		pHero->DisableAutoAttack(); //전문 스킬을 쓰면 무조건 자동어택 기능을 끈다.

		int nSkillLevel = pHero->GetMugongLevel(pSkillInfo->GetSkillIndex());

		if( GetObjectKindGroup(wObjectKind) == eOBJECTKINDGROUP_NONE )
		{
			CHATMGR->AddMsg(CTC_SYSMSG, "ΝА㎌�胥Aㄳ�鋤弔뻘甸� ");						
			return FALSE;
		}

		//타이탄 탑승시 전문스킬 발동 불가 처리
		if(pHero->InTitan() == TRUE)
		{
			CHATMGR->AddMsg( CTC_SYSMSG, CHATMGR->GetChatMsg(1657) );
			return FALSE;
		}

		//스킬종류와 몬스터 종류와의 비교
		BOOL bJobSkillSuccess = FALSE;
		switch(wSkillKind)
		{
		case SKILLKIND_MINING:	// 채광
			{			
				if( wObjectKind == eObjectKind_Mining )
				{
					bJobSkillSuccess = TRUE;
				}				
			}
			break;
		case SKILLKIND_COLLECTION:	// 채집
			{
				if( wObjectKind == eObjectKind_Collection )
				{
					bJobSkillSuccess = TRUE;
				}				
			}
			break;
		case SKILLKIND_HUNT:	// 사냥
			{
				if( wObjectKind == eObjectKind_Hunt )
				{
					bJobSkillSuccess = TRUE;
				}				
			}
			break;
		}

		if(bJobSkillSuccess == FALSE)
		{
			CHATMGR->AddMsg(CTC_SYSMSG, "ㄳ촑뱄윰ず㎌�� ");								
			return FALSE;
		}

		CMonster* pMonster = (CMonster*)pObject;
		if(pMonster == NULL)	//예외처리
		{				
			return FALSE;
		}

		if(nSkillLevel < pMonster->GetMonsterLevel())
		{
			CHATMGR->AddMsg(CTC_SYSMSG, "㎌�碩��탁L쬍 ");
			return FALSE;
		}		
	}
	else if( GetObjectKindGroup(wObjectKind) == eOBJECTKINDGROUP_JOB )
	{
		CHATMGR->AddMsg(CTC_SYSMSG, "Ω셸ㄳ�黍嘔� ");
		pHero->DisableAutoAttack(); //전문 스킬을 쓰면 무조건 자동어택 기능을 끈다.		
		return FALSE;
	}	
	
	return TRUE;
}

BOOL CSkillManager::CheckSkillKind(WORD wSkillKind)
{
	if((wSkillKind == SKILLKIND_MINING) || (wSkillKind == SKILLKIND_COLLECTION) || (wSkillKind == SKILLKIND_HUNT))	
	{
		return TRUE;
	}

	return FALSE;
}
///////////////////////////////////////////////////////////////////////////////

///// 2007. 10. 15. CBH - 타이탄 무공과 무기 체크(SkillInfo에서 이동) /////////////
BOOL CSkillManager::CheckTitanWeapon(CHero* pHero, SKILLINFO* SkillInfo)
{
	// magi82 - Titan(070912) 타이탄 무공업데이트
	// 타이탄에 탑승중일때..
	if( pHero->InTitan() == TRUE )
	{
		// 타이탄 무공이면 타이탄 무기를 체크한다.
		if( SkillInfo->SkillKind == SKILLKIND_TITAN )
		{
			if( SkillInfo->WeaponKind != 0 )
			{
				if( pHero->GetTitanWeaponEquipType() != SkillInfo->WeaponKind )
				{
					CHATMGR->AddMsg( CTC_SYSMSG, CHATMGR->GetChatMsg(151) );
					pHero->DisableAutoAttack();
					return FALSE;
				}

				if( pHero->GetWeaponEquipType() != pHero->GetTitanWeaponEquipType() )
				{
					CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(1644));
					pHero->DisableAutoAttack();
					return FALSE;
				}
			}
		}
		else	// 타이탄 무공이 아니면 에러
		{
			if(SKILLMGR->CheckSkillKind(SkillInfo->SkillKind) == TRUE)
			{
				CHATMGR->AddMsg( CTC_SYSMSG, CHATMGR->GetChatMsg(1657) );
			}
			else
			{
				CHATMGR->AddMsg( CTC_SYSMSG, CHATMGR->GetChatMsg(1653) );
			}			
			return FALSE;
		}
	}
	else	// 타이탄에 탑승중이지 않을때..
	{
		// 타이탄 무공이면 에러
		if( SkillInfo->SkillKind == SKILLKIND_TITAN )
		{
			CHATMGR->AddMsg( CTC_SYSMSG, CHATMGR->GetChatMsg(1668) );
			return FALSE;
		}
		else	// 타이탄 무공이 아니면 캐릭터 무기를 체크한다.
		{
			if( SkillInfo->WeaponKind != 0 )
			{
				if( pHero->GetWeaponEquipType() != SkillInfo->WeaponKind )
				{
					CHATMGR->AddMsg( CTC_SYSMSG, CHATMGR->GetChatMsg(151) );
					return FALSE;
				}
			}
		}
	}

	return TRUE;
}
////////////////////////////////////////////////////////////////////////////////////////

#endif
