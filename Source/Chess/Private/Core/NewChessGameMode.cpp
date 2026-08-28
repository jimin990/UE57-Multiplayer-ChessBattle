// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/NewChessGameMode.h"
#include "GameFramework/GameStateBase.h"
#include "Character/Players/MyChessPlayerController.h"
#include "Character/Players/ChessCameraPawn.h"
#include "Character/ChessPiece/ChessPieceBase.h"
#include "System/ChessBoard.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Core/ChessGameState.h"
#include "System/ChessMoveFunc.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"

ANewChessGameMode::ANewChessGameMode()
{
	// SeamTaval
	bUseSeamlessTravel = true;

	//DefaultPawnClass 은 BP에서 처리

	//BP에서 PlayerControllerClass 설정 = 메인 화면이랑 클래스랑 같은 것으로 맞추기
	//게임모드에 GameMode 넣기
	UE_LOG(LogTemp, Warning, TEXT("GM_________________________________"));
}

void ANewChessGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	UE_LOG(LogTemp, Warning, TEXT("GM - PreLogin!"));
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
}

APlayerController* ANewChessGameMode::Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	UE_LOG(LogTemp, Warning, TEXT("GM - Login!"));
	return Super::Login(NewPlayer, InRemoteRole, Portal, Options, UniqueId, ErrorMessage);
}

void ANewChessGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	UE_LOG(LogTemp, Warning, TEXT("1!!"));

	UWorld* GW = GetWorld();

	if (GW)
	{
		UE_LOG(LogTemp, Warning, TEXT("World!"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("World! is null"));
	}
}

void ANewChessGameMode::InitGameState()
{
	Super::InitGameState();

	UE_LOG(LogTemp, Warning, TEXT("2!!"));
	
	AChessGameState* GS = Cast<AChessGameState>(GetWorld()->GetGameState());

	if (GS)
	{
		UE_LOG(LogTemp, Warning, TEXT("GS!!"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("GS is null"));
	}

	//델리게이트 호출 
}

void ANewChessGameMode::ReadyPlayer()
{
	UE_LOG(LogTemp, Warning, TEXT("Ready"));
	ReadyToPlayerNum++;
	
	if (ReadyToPlayerNum >= 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ready to all"));
		TryStartChessGame();
	}
}

void ANewChessGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("GM - BeginPlay()!!"));
}


// 이건 SeamlessTravel만 동작
void ANewChessGameMode::PostSeamlessTravel()
{	
	UE_LOG(LogTemp, Warning, TEXT("PostSeamlessTravel"));
	Super::PostSeamlessTravel();
}

// 이건 SeamlessTravel만 동작
void ANewChessGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
	AMyChessPlayerController* PC =
		Cast<AMyChessPlayerController>(C);

	if (!PC)
	{
		return;
	}

	// 플레이어 넣기
	Players.AddUnique(PC);

	if (WhitePlayer == nullptr)
	{
		WhitePlayer = PC;
		PC->PlayerTeam = EChessTeam::White;
		UE_LOG(LogTemp, Warning, TEXT("Assigned WHITE to %s"), *GetNameSafe(PC));
	}
	else if (BlackPlayer == nullptr && PC != WhitePlayer)
	{
		BlackPlayer = PC;
		PC->PlayerTeam = EChessTeam::Black;
		UE_LOG(LogTemp, Warning, TEXT("Assigned BLACK to %s"), *GetNameSafe(PC));
	}

	UE_LOG(LogTemp, Warning, TEXT("HandleSeamlessTravelPlayer"));
	Super::HandleSeamlessTravelPlayer(C);
}


// 원해 안쓰다가 추가했음
void ANewChessGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

AActor* ANewChessGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	return Super::FindPlayerStart_Implementation(Player, IncomingName);
}

AActor* ANewChessGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	UE_LOG(LogTemp, Warning, TEXT("ChoosePlayerStart1"));

	AMyChessPlayerController* PC = Cast<AMyChessPlayerController>(Player);

	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("ChoosePlayerStart: Not a Chess PC"));
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	UE_LOG(LogTemp, Warning, TEXT("ChoosePlayerStart | PC=%s | Team=%d"),
		*GetNameSafe(PC), (int32)PC->PlayerTeam);

	const FName DesiredTag = (PC->PlayerTeam == EChessTeam::White)
		? FName("WhiteSpawnLocation")
		: FName("BlackSpawnLocation");

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		if (APlayerStart* Start = Cast<APlayerStart>(Actor))
		{
			if (Start->PlayerStartTag == DesiredTag)
			{
				UE_LOG(LogTemp, Warning, TEXT("CameraPawn Rot: %s"),
					*Start->GetActorRotation().ToString());

				return Start;
			}
		}
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

void ANewChessGameMode::RestartPlayer(AController* NewPlayer)
{
	UE_LOG(LogTemp, Warning, TEXT("RestartPlayer"));
	Super::RestartPlayer(NewPlayer);

	if (AMyChessPlayerController* PC = Cast<AMyChessPlayerController>(NewPlayer))
	{
		AActor* StartSpot = FindPlayerStart(PC);

		if (!StartSpot)
		{
			return;
		}

		const FRotator StartRot = StartSpot->GetActorRotation();

		if (PC->PlayerTeam == EChessTeam::White)
		{
			WhiteCamera = Cast<AChessCameraPawn>(PC->GetPawn());
			WhiteCamera->SetActorRotation(StartRot);
		}
		else if (PC->PlayerTeam == EChessTeam::Black)
		{
			BlackCamera = Cast<AChessCameraPawn>(PC->GetPawn());
			BlackCamera->SetActorRotation(StartRot);
		}
	}

	//TryStartChessGame();
}

void ANewChessGameMode::TryStartChessGame()
{
	if (bChessGameStarted)
	{
		return;
	}

	if (!WhitePlayer || !BlackPlayer)
	{
		return;
	}

	if (!WhiteCamera || !BlackCamera)
	{
		return;
	}

	bChessGameStarted = true;
	
	// 플레이어 준비가 완료되면 게임 InitGame으로 변경
	ChangeGamePhase(EGamePhase::InitGamePhase);
}

// 코드 리팩토링 시급
void ANewChessGameMode::TryMovePiece(AChessPieceBase* Piece, AMyChessPlayerController* TryPlayer, int32 MoveIndex)
{
	// 서버만 이동 관리
	if (!HasAuthority())
	{
		return;
	}

	AChessGameState* GS =
		Cast<AChessGameState>(
			GetWorld()->GetGameState()
		);

	if (!GS) return;

	//자신의 턴이 아닌 경우
	if (GS->CurrentTurnTeam != TryPlayer->PlayerTeam)
	{
		return;
	}

	// 이동 가능 여부 체크
	if (!CheckInvalidMove(Piece, MoveIndex))
	{
		return;
	}

	FTransform Transform;
	GS->ChessBoard->ISM->GetInstanceTransform(MoveIndex, Transform, true);
	FVector MoveLoc = Transform.GetLocation();

	//타일 위로 위치 조정
	MoveLoc += FVector3d(0, 0, 180.f);

	// 자리에 아무런 상대가 없다면
	if (GS->ChessPieces[MoveIndex] == nullptr)
	{
		//이전에 있던 위치 초기화
		GS->ChessPieces[Piece->OwnIndex] = nullptr;
		Piece->SetActorLocation(MoveLoc);
		GS->ChessPieces[MoveIndex] = Piece;
		Piece->OwnIndex = MoveIndex;
	}
	else // 적팀 이 있을 경우 배틀 준비
	{
		//BettingBattleTime(Attacker, Defender);

		// 전투 세팅
		SetBattle(Piece, GS->ChessPieces[MoveIndex], MoveIndex);
		return;
	}

	// 다음 팀 턴으로 변경
	GS->CurrentTurnTeam = GS->CurrentTurnTeam == EChessTeam::White ? EChessTeam::Black : EChessTeam::White;
}

bool ANewChessGameMode::CheckInvalidMove(AChessPieceBase* Piece, int32 MoveIndex)
{
	AChessGameState* GS = Cast<AChessGameState>(GetWorld()->GetGameState());

	TArray<int32> Moves =
		UChessMoveFunc::GetPieceMoves(Piece, GS->ChessPieces);

	return Moves.Contains(MoveIndex);
}

void ANewChessGameMode::SetBattle(AChessPieceBase* InAttacker, AChessPieceBase* InDefender, int32 InBattleIndex)
{
	AChessGameState* GS = Cast<AChessGameState>(GetWorld()->GetGameState());
	if (!GS) return;

	// 현재 공격자와 수비자 지정
	GS->Attacker = InAttacker;
	GS->Defender = InDefender;

	// 전투 벌어진 공간 지정
	GS->BattleIndex = InBattleIndex;

	ChangeGamePhase(EGamePhase::BettingPhase);
}

void ANewChessGameMode::SetEndBattle()
{
	ChangeGamePhase(EGamePhase::EndBattlePhase);
}

void ANewChessGameMode::ChangeGamePhase(EGamePhase NewPhase)
{
	// 서버에서만 실행
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ChangeGamePhase] Called on Client. Ignored."));
		return;
	}

	AChessGameState* GS = GetGameState<AChessGameState>();

	if (!GS)
	{
		UE_LOG(LogTemp, Error, TEXT("[ChangeGamePhase] GameState is NULL!"));
		return;
	}

	// 같은 페이즈로 중복 변경 방지
	if (GS->CurGamePhase == NewPhase)
	{
		return;
	}

	UEnum* EnumPtr = StaticEnum<EGamePhase>();

	FString Name = EnumPtr->GetNameStringByValue((int64)NewPhase);

	UE_LOG(LogTemp, Warning, TEXT("Game Changed to %s"), *Name);

	// 실제 게임 페이즈 변경
	// 호스트, 클라이언트 둘다 델리게이트를 보내기 위해
	GS->SetGamePhase(NewPhase);

	switch (NewPhase)
	{
	case EGamePhase::InitGamePhase:
		/*
			체스판 생성 후 다음 단계
		*/
		InitGamePhase();
		break;

	case EGamePhase::PreGamePhase:
		/*
			게임 시작
			체스말 생성
			체스 말 스폰 2초

		*/
		PreGamePhase();
		break;

	case EGamePhase::ChessPhase:
		ChessPhase();
		break;

	case EGamePhase::BettingPhase:
		BettingPhase();
		break;

	case EGamePhase::PreBattlePhase:
		PreBattlePhase();
		break;

	case EGamePhase::BattlePhase:
		BattlePhase();
		break;

	case EGamePhase::EndBattlePhase:
		EndBattlePhase();
		break;

	case EGamePhase::FinishBattlePhase:
		FinishBattlePhase();
		break;

	case EGamePhase::GameEndPhase:
		break;
	}
}

void ANewChessGameMode::InitGamePhase()
{
	// 아직 로딩 화면

	AChessGameState* GS = Cast<AChessGameState>(GetWorld()->GetGameState());
	if (!GS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InitGame] GS is null!"));
		return;
	}

	// 현재 전투 필드에 전투가 없음을 초기화
	GS->BattleIndex = -1;

	// 배틀중이 아니지만 게임 시작 전 시간이 흐르는 것을 막기 위해 true 임시 값
	GS ->isBattle = true;

	// 체스판 스폰
	SpawnChessBoard();

	// 우선 여기서 체스말 스폰
	SpawnChessPieces();

	// 만약 컨트롤러 설정이 완료됬다는 값을 받을 수 있다면 여기서 받고 넘어가기

	// 임시로 3초로 지정 - 동기화 문제
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this]()
		{
			// 초기화 후 다음 단계로
			ChangeGamePhase(EGamePhase::PreGamePhase);
		}, 3, false);
}

void ANewChessGameMode::SpawnChessBoard()
{
	TArray<AActor*> FoundActors;

	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		APlayerStart::StaticClass(),
		FoundActors
	);


	FTransform Transform1;

	for (AActor* Actor : FoundActors)
	{
		APlayerStart* PlayerStart = Cast<APlayerStart>(Actor);

		if (PlayerStart && PlayerStart->PlayerStartTag == FName("ChessBoardSpawnLocation"))
		{
			Transform1 = PlayerStart->GetTransform();
		}
	}

	FActorSpawnParameters Params;

	if (!ChessBoardClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("BoardClass is null"));
	}
	else
	{
		//게임 스테이트에 체스 보드 넣어주기
		AChessGameState* GS =
			Cast<AChessGameState>(
				GetWorld()->GetGameState()
			);

		GS->ChessBoard = GetWorld()->SpawnActor<AChessBoard>(
			ChessBoardClass,
			Transform1.GetLocation(),
			Transform1.Rotator(),
			Params
		);

		// 보드를 연결
		WhitePlayer->Board = GS->ChessBoard;
		BlackPlayer->Board = GS->ChessBoard;
	}
}

void ANewChessGameMode::CheckTurnTime()
{
	// 서버만 시간 관리
	if (!HasAuthority())
	{
		return;
	}

	AChessGameState* GS = Cast<AChessGameState>(GetWorld()->GetGameState());
	if (!GS) return;

	// 배틀시간 시간 카운터 X
	if (GS->isBattle) return;

	GS->SetTime(0.5f);

	if (GS->WhiteRemainTime <= 0 || GS->BlackRemainTime <= 0)
	{
		ChangeGamePhase(EGamePhase::GameEndPhase);
	}
}

void ANewChessGameMode::PreGamePhase()
{
	// 컨트롤러로 부터 페이드 아웃 다됬다는 알림 받는 것 추가

	// 체스 말 소환
	//SpawnChessPieces();

	//게임 시간 함수 ON
	FTimerHandle TurnTimerHandle;
	GetWorldTimerManager().SetTimer(
		TurnTimerHandle,
		this,
		&ANewChessGameMode::CheckTurnTime,
		0.5f,
		true
	);

	// 임시로 3초로 지정 - 체스 소환 연출 (예상 시간) X 시작 알림 
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this]()
		{
			// 임시 첫턴 정해주기 - 나중에 위젯에 턴 변경 델리게이트로 따로 빼서 관리
			AChessGameState* GS = Cast<AChessGameState>(GetWorld()->GetGameState());
			if (!GS) return;
			GS->CurrentTurnTeam = EChessTeam::White;

			// 초기화 후 다음 단계로
			ChangeGamePhase(EGamePhase::ChessPhase);
		}, 3, false);
}

void ANewChessGameMode::SpawnChessPieces()
{
	AChessGameState* GS =
		GetGameState<AChessGameState>();

	if (!GS || !GS->ChessBoard)
	{
		return;
	}

	GS->ChessPieces.SetNum(64);

	// White
	SpawnPiece(0, EChessPieceType::Rook, EChessTeam::White);
	SpawnPiece(1, EChessPieceType::Knight, EChessTeam::White);
	SpawnPiece(2, EChessPieceType::Bishop, EChessTeam::White);
	SpawnPiece(3, EChessPieceType::Queen, EChessTeam::White);
	SpawnPiece(4, EChessPieceType::King, EChessTeam::White);
	SpawnPiece(5, EChessPieceType::Bishop, EChessTeam::White);
	SpawnPiece(6, EChessPieceType::Knight, EChessTeam::White);
	SpawnPiece(7, EChessPieceType::Rook, EChessTeam::White);

	for (int32 i = 8; i < 16; i++)
	{
		SpawnPiece(
			i,
			EChessPieceType::Pawn,
			EChessTeam::White
		);
	}

	// Black
	SpawnPiece(56, EChessPieceType::Rook, EChessTeam::Black);
	SpawnPiece(57, EChessPieceType::Knight, EChessTeam::Black);
	SpawnPiece(58, EChessPieceType::Bishop, EChessTeam::Black);
	SpawnPiece(59, EChessPieceType::Queen, EChessTeam::Black);
	SpawnPiece(60, EChessPieceType::King, EChessTeam::Black);
	SpawnPiece(61, EChessPieceType::Bishop, EChessTeam::Black);
	SpawnPiece(62, EChessPieceType::Knight, EChessTeam::Black);
	SpawnPiece(63, EChessPieceType::Rook, EChessTeam::Black);

	for (int32 i = 48; i < 56; i++)
	{
		SpawnPiece(
			i,
			EChessPieceType::Pawn,
			EChessTeam::Black
		);
	}
}

void ANewChessGameMode::SpawnPiece(int32 Index, EChessPieceType PieceType, EChessTeam Team)
{
	AChessGameState* GS =
		GetGameState<AChessGameState>();

	if (!GS || !GS->ChessBoard)
	{
		return;
	}

	FTransform TileTransform;

	GS->ChessBoard->ISM->GetInstanceTransform(
		Index,
		TileTransform,
		true
	);

	FVector SpawnLoc =
		TileTransform.GetLocation();

	SpawnLoc.Z += 180.f;

	FRotator SpawnRot =
		Team == EChessTeam::Black
		? FRotator(0.f, 0.f, 0.f)
		: FRotator(0.f, 180.f, 0.f);

	FActorSpawnParameters Params;

	Params.Owner =
		Team == EChessTeam::White
		? WhitePlayer
		: BlackPlayer;

	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod
		::AlwaysSpawn;

	if (!ChessPieceClasses[PieceType])
	{
		UE_LOG(LogTemp, Warning, TEXT("Not PieceClass"));
		return;
	}

	AChessPieceBase* NewPiece =
		GetWorld()->SpawnActor<AChessPieceBase>(
			ChessPieceClasses[PieceType],
			SpawnLoc,
			SpawnRot,
			Params
		);

	if (!NewPiece)
	{
		return;
	}

	NewPiece->PieceType = PieceType;
	NewPiece->Team = Team;
	NewPiece->OwnIndex = Index;

	GS->ChessPieces[Index] = NewPiece;

	//NewPiece->Multicast_ColorChanged();
	NewPiece->ApplyTeamColor();
}

void ANewChessGameMode::ChessPhase()
{
	AChessGameState* GS = Cast<AChessGameState>(GetWorld()->GetGameState());
	if (!GS) return;

	// 배틀 시간이 아님으로 시간이 흐른다.
	GS->isBattle = false;
}

void ANewChessGameMode::BettingPhase()
{
	AChessGameState* GS = Cast<AChessGameState>(GetWorld()->GetGameState());
	if (!GS) return;

	// 타이머 멈춤
	GS->isBattle = true;

	// 임시로 11초로 지정
	// 위젯 내려오는 시간까지
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this]()
		{
			// 배팅 결과를 확인
			CheckBettingResult();
		}, 10, false);
}

void ANewChessGameMode::CheckBettingResult()
{
	AChessGameState* GS = Cast<AChessGameState>(GetWorld()->GetGameState());
	if (!GS) return;

	// 각자 배팅 타임 확인 로그
	UE_LOG(LogTemp, Warning, TEXT("White %f"), GS->WhiteBattleTime);
	UE_LOG(LogTemp, Warning, TEXT("Black %f"), GS->BlackBattleTime);

	// 공격자 시간 보너스
	if (GS->Attacker->Team == EChessTeam::White)
	{
		GS->WhiteBattleTime += 20;
	}
	else
	{
		GS->BlackBattleTime += 20;
	}

	// 배팅을 안했다.
	if (GS->WhiteBattleTime == 0)
	{
		ChangeGamePhase(EGamePhase::FinishBattlePhase);
	}
	else if (GS->BlackBattleTime == 0)
	{
		ChangeGamePhase(EGamePhase::FinishBattlePhase);
	}
	else
	{
		// 부전승이 아니라면 전투 준비 단계로
		ChangeGamePhase(EGamePhase::PreBattlePhase);
	}
}

void ANewChessGameMode::PreBattlePhase()
{
	FTimerHandle TurnTimerHandle;
	GetWorldTimerManager().SetTimer(
		TurnTimerHandle,
		this,
		&ANewChessGameMode::CheckBattleTime,
		0.5f,
		true
	);

	AChessGameState* GS = Cast<AChessGameState>(GetWorld()->GetGameState());
	if (!GS) return;

	//FVector3d AttackerLocation;
	//FVector3d TargetLocation;
	FTransform AttackerLocation;
	FTransform TargetLocation;
	

	// 체스 말 스폰 포인트로 이동 
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundActors);
	for (AActor* Actor : FoundActors)
	{
		APlayerStart* PlayerStart = Cast<APlayerStart>(Actor);

		if (PlayerStart && PlayerStart->PlayerStartTag == FName("Attacker"))
		{
			//AttackerLocation = PlayerStart->GetActorLocation();
			AttackerLocation = PlayerStart->GetTransform();
		}

		if (PlayerStart && PlayerStart->PlayerStartTag == FName("Target"))
		{
			//TargetLocation = PlayerStart->GetActorLocation();
			TargetLocation = PlayerStart->GetTransform();
		}
	}

	GS->Attacker->SetActorLocation(AttackerLocation.GetLocation());
	GS->Attacker->SetActorRotation(AttackerLocation.GetRotation());
	GS->Defender->SetActorLocation(TargetLocation.GetLocation());
	GS->Defender->SetActorRotation(TargetLocation.GetRotation());

	// 각 컨트롤러 폰에 빙의 시키기
	if (GS->Attacker->Team == EChessTeam::Black)
	{
		BlackPlayer->Possess(GS->Attacker);
		WhitePlayer->Possess(GS->Defender);

	}
	else
	{
		WhitePlayer->Possess(GS->Attacker);
		BlackPlayer->Possess(GS->Defender);
	}

	// 체력 바 표시
	// 나중에 수정
	GS->Attacker->Multicast_ShowBattleWidget();
	GS->Defender->Multicast_ShowBattleWidget();

	// 3초 후에 전투 시작
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this]()
		{
			// 배팅 결과를 확인
			ChangeGamePhase(EGamePhase::BattlePhase);
		}, 3, false);
}

void ANewChessGameMode::CheckBattleTime()
{
	AChessGameState* GS = Cast<AChessGameState>(GetWorld()->GetGameState());
	if (!GS) return;

	// 배틀시간 시간 카운터 X
	if (!GS->isBattleStart) return;

	GS->SetBattleTime(0.5f);

	if (GS->WhiteBattleTime <= 0 || GS->BlackBattleTime <= 0)
	{
		SetEndBattle();
	}
}

void ANewChessGameMode::BattlePhase()
{
	AChessGameState* GS = Cast<AChessGameState>(GetWorld()->GetGameState());
	if (!GS) return;

	GS->isBattleStart = true;
}

void ANewChessGameMode::EndBattlePhase()
{
	// 배틀 승리 알리는 함수 구현

	// 배틀 종료
	AChessGameState* GS = Cast<AChessGameState>(GetWorld()->GetGameState());
	if (!GS) return;
	GS->isBattleStart = false;

	// 임시로 3초로 지정
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this]()
		{
			ChangeGamePhase(EGamePhase::FinishBattlePhase);
		}, 3, false);
}

void ANewChessGameMode::FinishBattlePhase()
{
	AChessGameState* GS = Cast<AChessGameState>(GetWorld()->GetGameState());
	if (!GS) return;

	//체력 바 표시 끄기
	GS->Attacker->Multicast_HideBattleWidget();
	GS->Defender->Multicast_HideBattleWidget();

	// 카메라로 Possess
	if (BlackPlayer->GetPawn() != BlackCamera)
	{
		BlackPlayer->Possess(BlackCamera);
	}
	
	if (WhitePlayer->GetPawn() != WhiteCamera)
	{
		WhitePlayer->Possess(WhiteCamera);
	}

	//이전 위치 필드 초기화
	GS->ChessPieces[GS->Attacker->OwnIndex] = nullptr;
	GS->ChessPieces[GS->Defender->OwnIndex] = nullptr;

	// 승패 체크
	// -1 : 오류, 0 무승부, 1: Attacker 승리, 2: Defender 승리 
	int32 WinResult= CheckBattleResult();

	FTransform Transform;
	GS->ChessBoard->ISM->GetInstanceTransform(GS->BattleIndex, Transform, true);
	FVector MoveLoc = Transform.GetLocation();
	MoveLoc.Z += 180.f;

	// 무승부 일 경우
	if (WinResult == 0)
	{

		// 나중에 수정
		if (GS->Attacker->PieceType == EChessPieceType::King || GS->Defender->PieceType == EChessPieceType::King)
		{
			ChangeGamePhase(EGamePhase::GameEndPhase);
		}

		// 무승부인 경우 Defender 차례
		GS->CurrentTurnTeam = GS->Defender->Team;

		// 두 폰 삭제
		GS->Attacker->Destroy();
		GS->Defender->Destroy();

		// 전장 상태 정리
		GS->ChessPieces[GS->BattleIndex] = nullptr;

		//////// 나중에 수정
			//  승패 결과를 전송
		GS->Multicast_BattleResult(0, 0);

	}
	else if (WinResult == 1) // Attacker 승리
	{
		if (GS->Defender->PieceType == EChessPieceType::King)
		{
			ChangeGamePhase(EGamePhase::GameEndPhase);
		}

		//진 사람 한테 턴 주기
		GS->CurrentTurnTeam = GS->Defender->Team;

		GS->Attacker->SetActorLocation(MoveLoc);

		// 만약 팀 별로 방향 조절 
		// Unpossess로 풀린 권한 팀 별로 다시 부여
		if (GS->Attacker->Team == EChessTeam::White)
		{
			GS->Attacker ->SetActorRotation(FRotator(0.f, -180.f, 0.f));
			
			//Unpossess로 풀린 권한 다시 부여
			GS->Attacker->SetOwner(WhitePlayer);

			// 이긴 팀 남은 절반 시간 돌려주기
			GS->WhiteRemainTime += (GS->WhiteBattleTime / 2);

			//////// 나중에 수정
			//  승패 결과를 전송
			GS->Multicast_BattleResult(1, (GS->WhiteBattleTime / 2));
		}
		else
		{
			GS->Attacker->SetActorRotation(FRotator(0.f, 0.f, 0.f));
			//Unpossess로 풀린 권한 다시 부여
			GS->Attacker->SetOwner(BlackPlayer);

			// 이긴 팀 남은 절반 시간 돌려주기
			GS->BlackRemainTime += (GS->BlackBattleTime / 2);

			//////// 나중에 수정
			//  승패 결과를 전송
			GS->Multicast_BattleResult(2, (GS->BlackBattleTime / 2));
		}

		GS->Defender->Destroy();

		GS->ChessPieces[GS->BattleIndex] = GS->Attacker;
		GS->Attacker->OwnIndex = GS->BattleIndex;
	}
	else if (WinResult == 2) // Defender 승리
	{
		if (GS->Attacker->PieceType == EChessPieceType::King)
		{
			ChangeGamePhase(EGamePhase::GameEndPhase);
		}

		//진 사람 한테 턴 주기
		GS->CurrentTurnTeam = GS->Attacker->Team;

		GS->Defender->SetActorLocation(MoveLoc);

		// 만약 팀 별로 방향 조절 
		// Unpossess로 풀린 권한 팀 별로 다시 부여
		if (GS->Defender->Team == EChessTeam::White)
		{
			GS->Defender->SetActorRotation(FRotator(0.f, -180.f, 0.f));

			//Unpossess로 풀린 권한 다시 부여
			GS->Defender->SetOwner(WhitePlayer);

			// 이긴 팀 남은 절반 시간 돌려주기
			GS->WhiteRemainTime += (GS->WhiteBattleTime / 2);

			//////// 나중에 수정
			//  승패 결과를 전송
			GS->Multicast_BattleResult(1, (GS->WhiteBattleTime / 2));
		}
		else
		{
			GS->Defender->SetActorRotation(FRotator(0.f, 0.f, 0.f));
			//Unpossess로 풀린 권한 다시 부여
			GS->Defender->SetOwner(BlackPlayer);

			// 이긴 팀 남은 절반 시간 돌려주기
			GS->BlackRemainTime += (GS->BlackBattleTime / 2);

			//////// 나중에 수정
			//  승패 결과를 전송
			GS->Multicast_BattleResult(2, (GS->BlackBattleTime / 2));
		}

		GS->Attacker->Destroy();

		GS->ChessPieces[GS->BattleIndex] = GS->Defender;
		GS->Defender->OwnIndex = GS->BattleIndex;
	}

	// 전투 상태 초기화 하기
	GS->BattleIndex = -1;
	GS->Attacker = nullptr;
	GS->Defender = nullptr;
	GS->WhiteBattleTime = 0;
	GS->BlackBattleTime = 0;

	// 임시로 1초로 지정
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this]()
		{
			ChangeGamePhase(EGamePhase::ChessPhase);
		}, 3, false);
}

// 반드시 나중에 return EChessTeam::None 이거 수정!!!!
int32 ANewChessGameMode::CheckBattleResult()
{
	// 이전 호출한 플레이가 체크 중일때
	//if (isChecking) EChessTeam::None;

	AChessGameState* GS = Cast<AChessGameState>(GetWorld()->GetGameState());
	// 오류 반환
	if (!GS) return -1;

	// 시간 체크
	// 둘다 시간이 없다면
	if (GS->WhiteBattleTime == 0 && GS->BlackBattleTime == 0)
	{
		// 무승부 반환
		return 0;
	}
	else if (GS->WhiteBattleTime == 0) // 백팀 시간이 없을 경우
	{
		// 백팀이 Attacker라면
		if (GS->Attacker->Team == EChessTeam::White)
		{
			// Defender 승리 반환
			return 2;
		}
		else
		{
			// 아니면 Attacker 승리 반환
			return 1;
		}
	}
	else if (GS->BlackBattleTime == 0) // 흑팀 시간이 없을때
	{
		// 흑팀이 Attacker라면
		if (GS->Attacker->Team == EChessTeam::Black)
		{
			// Defender 승리 반환
			return 2;
		}
		else
		{
			// 아니면 Attacker 승리 반환
			return 1;
		}
	}

	// 체력 체크 
	if (GS->Attacker->CurrentHP <= 0 && GS->Defender->CurrentHP <= 0)
	{
		// 무승부 반환
		return 0;
	}
	else if (GS->Attacker->CurrentHP <= 0)
	{
		return 2;
	}
	else if (GS->Defender->CurrentHP <= 0)
	{
		return 1;
	}

	// 나중에 전투 결과  ENUM으로 변경
	UE_LOG(LogTemp, Warning, TEXT("The battle isn't over yet."));
	return -1;
}

void ANewChessGameMode::GameEndPhase()
{
	UE_LOG(LogTemp, Warning,
		TEXT("Win! %s"),
		*UEnum::GetValueAsString(GameResult));
}

void ANewChessGameMode::PlayGlobalFade()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AMyChessPlayerController* PC = Cast<AMyChessPlayerController>(It->Get());
		if (PC)
		{
			PC->Client_FadeInOut();
		}
	}
}


