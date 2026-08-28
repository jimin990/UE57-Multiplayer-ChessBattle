// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ChessPiece/ChessPieceBase.h"
#include "Character/Players/MyChessPlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimInstance.h"
#include "Core/NewChessGameMode.h"
#include "Core/ChessGameState.h"
#include "System/ChessBoard.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "EnhancedInputSubsystemInterface.h"
#include "Components/WidgetComponent.h"
#include "UI/PieceBattleWidget.h"
#include "Data/PiecePrimaryDataAsset.h"

// Sets default values
AChessPieceBase::AChessPieceBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	NetDormancy = ENetDormancy::DORM_Never;
	RootComponent = GetCapsuleComponent();

	// 콤보 공격
	PieceComboIndex = 0;
	bIsAttacking = false;
	bComboInput = false;

	GetCapsuleComponent()->InitCapsuleSize(84.0f, 180.0f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
	GetCapsuleComponent()->SetCollisionResponseToChannel(
		ECC_Pawn,
		ECR_Block
	);

	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimBP(
		TEXT("/Game/Chess/ChessGame/ChessPieces/Anims/ABP_ChessPiece")
	);

	if (AnimBP.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(AnimBP.Class);
	}

	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	GetMesh()->SetRelativeRotation(FRotator(0,-90,0));

	GetMesh()->SetWorldScale3D(FVector(2.0f, 2.0f, 2.0f));

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmComponent->SetupAttachment(GetRootComponent());
	SpringArmComponent->SetRelativeLocation(FVector(0.f, 0.f, 150.f)); // 머리 높이

	SpringArmComponent->TargetArmLength = 600.f;
	SpringArmComponent->bUsePawnControlRotation = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(SpringArmComponent);
	CameraComponent->bUsePawnControlRotation = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->JumpZVelocity = 500.f; // 점프 속도
	GetCharacterMovement()->AirControl = 0.5f; // 0~1, 공중 이동 민감도

	// 임시로 위치 초기화
	OwnIndex = 0;

	// 컨트롤러 즉 Possess가 안되있더라도 이동가능
	GetCharacterMovement()->bRunPhysicsWithNoController = true;

	//Input
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> PieceIMCPtr(TEXT("/Game/Chess/ChessGame/ChessPieces/Input/IMC_ChessPieceBase"));
	if (PieceIMCPtr.Succeeded())
	{
		PieceIMC = PieceIMCPtr.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> LookActionPtr(TEXT("/Game/Chess/ChessGame/ChessPieces/Input/IA_Look"));
	if (LookActionPtr.Succeeded())
	{
		LookAction = LookActionPtr.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionPtr(TEXT("/Game/Chess/ChessGame/ChessPieces/Input/IA_Move"));
	if (MoveActionPtr.Succeeded())
	{
		MoveAction = MoveActionPtr.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> JumpActionPtr(TEXT("/Game/Chess/ChessGame/ChessPieces/Input/IA_Jump"));
	if (JumpActionPtr.Succeeded())
	{
		JumpAction = JumpActionPtr.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> DeadMontagePtr(TEXT("/Game/Chess/ChessGame/ChessPieces/Anims/Montages/AM_ChessDead"));
	if (DeadMontagePtr.Succeeded())
	{
		DeadMontage = DeadMontagePtr.Object;
	}

	//UI
	PieceBattleWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("Widget"));

	PieceBattleWidgetComp->SetupAttachment(RootComponent);

	PieceBattleWidgetComp->SetRelativeLocation(FVector(0,0,210));

	// 항성 카메라를 위젯이 보게 ------------------------- 나중에 정리
	PieceBattleWidgetComp ->SetWidgetSpace(EWidgetSpace::Screen);
	static ConstructorHelpers::FClassFinder<UPieceBattleWidget> PieceBattleWidgetClassRef(TEXT("/Game/Chess/ChessGame/Widget/WBP_PieceBattleWidget"));
	if (PieceBattleWidgetClassRef.Succeeded())
	{
		PieceBattleWidgetClass = PieceBattleWidgetClassRef.Class;
	}

	PieceBattleWidgetComp->SetVisibility(false);

	// Color
	static ConstructorHelpers::FObjectFinder<UMaterialInstance> WhiteMaterialPtr(TEXT("/Game/Chess/ChessGame/ChessPieces/Mesh/MI_White1"));
	if (WhiteMaterialPtr.Succeeded())
	{
		WhiteMaterial = WhiteMaterialPtr.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInstance> BlackMaterialPtr(TEXT("/Game/Chess/ChessGame/ChessPieces/Mesh/MI_Black"));
	if (BlackMaterialPtr.Succeeded())
	{
		BlackMaterial = BlackMaterialPtr.Object;
	}
}

// Called when the game starts or when spawned
void AChessPieceBase::BeginPlay()
{
	Super::BeginPlay();

	if (PieceBattleWidgetClass)
	{
		PieceBattleWidgetComp->SetWidgetClass(
			PieceBattleWidgetClass);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PieceBattleWidgetComp is null"));
	}

	PieceBattleWidgetComp->SetDrawAtDesiredSize(true);

	if (UPieceBattleWidget* Widget =
		Cast<UPieceBattleWidget>(
			PieceBattleWidgetComp
			->GetUserWidgetObject()))
	{
		Widget->InitWidget(this);

		OnHPChanged.AddDynamic(
			Widget,
			&UPieceBattleWidget::UpdateHp
		);
	}

	// 데이터 값 적용
	if (PieceData)
	{
		AttackMontage = PieceData->AttackMontage;
		Stat = PieceData->PieceStat;
		PieceType = PieceData->PieceType;
		GetMesh()->SetSkeletalMesh(PieceData->Mesh);

		// 최고 속도
		GetCharacterMovement()->MaxWalkSpeed = PieceData->PieceStat.MoveSpeed;        

		CurrentHP = PieceData->PieceStat.MaxHP;
		// 색상 변경
		ApplyTeamColor();
		//ColorChange();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PieceData is null"));
	}
}

// Called every frame
void AChessPieceBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AChessPieceBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	//UE_LOG(LogTemp, Warning, TEXT("SetupInput %s %d"), *GetName(), IsLocallyControlled());

	Super::SetupPlayerInputComponent(PlayerInputComponent);

	AMyChessPlayerController* PC = Cast<AMyChessPlayerController>(GetController());
	
	if (!PC->PieceIMC)
	{
		UE_LOG(LogTemp, Warning, TEXT("IMC is Null"));
		return;
	}

	if (!LookAction)
	{
		UE_LOG(LogTemp, Warning, TEXT("ClickAction && MoveAction is Null"));
		return;
	}

	if (!PC) return;

	if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(PC->PieceIMC, 0);
		}
	}

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EIC->ClearActionBindings();
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AChessPieceBase::Look);
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AChessPieceBase::Move);
		EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &AChessPieceBase::DoJumpStart);
		EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &AChessPieceBase::DoJumpEnd);
		EIC->BindAction(AttackAction, ETriggerEvent::Started, this, &AChessPieceBase::Attack);
	}
	
}

void AChessPieceBase::OnRep_Team()
{
	ApplyTeamColor();
}

void AChessPieceBase::OnRep_CurrentHP()
{
	OnHPChanged.Broadcast(CurrentHP);
}

void AChessPieceBase::Look(const FInputActionValue& Value)
{
	FVector2D Axis = Value.Get<FVector2D>();

	// Controller가 존재할 때만 처리
	if (Controller != nullptr)
	{
		AddControllerYawInput(Axis.X);
		AddControllerPitchInput(-Axis.Y);
	}
}

void AChessPieceBase::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();

	const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, Axis.Y);
	AddMovementInput(Right, Axis.X);
}

void AChessPieceBase::DoJumpStart()
{
	Jump();
}

void AChessPieceBase::DoJumpEnd()
{
	StopJumping();
}

void AChessPieceBase::Attack()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!Move) return;

	// 점프/낙하 중이면 공격 막기
	if (Move->IsFalling())
	{
		UE_LOG(LogTemp, Warning, TEXT("Can't Attack While Falling"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Attack - Client Input"));
	Server_Attack();
}

void AChessPieceBase::Server_Attack_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("Server_Attack"));

	if (bIsAttacking)
	{
		// 콤보 입력 버퍼링
		bComboInput = true;
		return;
	}

	// 새 콤보 시작
	PieceComboIndex = 1;
	StartCombo();
}

void AChessPieceBase::StartCombo()
{
	bIsAttacking = true;
	bComboInput = false;

	// 공격 중 이동 완전 정지
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_None);
	}

	Multicast_PlayComboMontage(PieceComboIndex);
}

void AChessPieceBase::Multicast_PlayComboMontage_Implementation(int32 ComboIndex)
{
	if (!AttackMontage) return;

	FName SectionName;
	switch (ComboIndex)
	{
	case 1: SectionName = "Combo1"; break;
	case 2: SectionName = "Combo2"; break;
	case 3: SectionName = "Combo3"; break;
	case 4: SectionName = "Combo4"; break;
	default: SectionName = "Combo1"; break;
	}

	PlayAnimMontage(AttackMontage, 1.f, SectionName);
}

// ==================== 핵심 수정 부분 ====================
void AChessPieceBase::AnimNotify_ComboCheck()
{
	if (!HasAuthority()) return;           // ★ 반드시 추가

	UE_LOG(LogTemp, Warning, TEXT("ComboCheck - Server"));

	if (bComboInput)
	{
		bComboInput = false;
		PieceComboIndex++;

		if (PieceComboIndex <= MaxCombo)
		{
			// 다음 콤보 몽타주 재생
			Multicast_PlayComboMontage(PieceComboIndex);
		}
	}
}
// =========================================================

void AChessPieceBase::AnimNotify_EndCombo()
{
	if (!HasAuthority()) return;

	UE_LOG(LogTemp, Warning, TEXT("EndCombo - Server"));

	bIsAttacking = false;
	bComboInput = false;
	PieceComboIndex = 0;

	// 이동 다시 허용
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Walking);   // 또는 MOVE_NavWalking
	}
}

void AChessPieceBase::AnimNotify_AttackHit()
{
	if (!HasAuthority()) return;

	// ... 기존 Hit 판정 코드 (그대로 사용)

	if (!HasAuthority()) return;
	FVector Start = GetActorLocation();
	FVector End = Start + GetActorForwardVector() * 150.f;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(60.f);
	TArray<FHitResult> Hits;
	bool bHit = GetWorld()->SweepMultiByChannel(
		Hits,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		Sphere
	);
	if (bHit)
	{
		for (auto& Hit : Hits)
		{
			AActor* Target = Hit.GetActor();
			if (Target && Target != this)
			{
				UGameplayStatics::ApplyDamage(
					Target,
					20.f,
					GetController(),
					this,
					nullptr
				);
			}
		}
	}
}

void AChessPieceBase::Die()
{

	if (!HasAuthority()) return;

	if (bIsDead) return;           // 이미 죽은 상태면 무시

	bIsDead = true;

	// 모든 클라이언트에게 죽음 몽타주 재생 요청
	Multicast_PlayDeathMontage();

	// 이동 완전 정지 (선택)
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_None);
	}
}

void AChessPieceBase::OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!HasAuthority()) return;

	UE_LOG(LogTemp, Warning, TEXT("Death Montage Finished"));
	// 여기서 추가로 하고 싶은 작업이 있으면 작성 (예: Destroy, Ragdoll 전환 등)
}

void AChessPieceBase::Multicast_PlayDeathMontage_Implementation()
{
	if (!DeadMontage) return;

	// 죽음 몽타주 재생
	//PlayAnimMontage(DeadMontage);

	// 이동 완전 정지 (선택)
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_None);
	}
	// 몽타주 끝난 시점 감지
	if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AChessPieceBase::OnDeathMontageEnded);
		AnimInst->Montage_SetEndDelegate(EndDelegate, DeadMontage);
	}
}

bool AChessPieceBase::Server_Dead_Validate()
{
	return true;
}

void AChessPieceBase::Server_Dead_Implementation()
{
	bIsDead = true;

	Multicast_PlayDeadMontage();

	// 입력 차단
	bIsAttacking = false;
	bComboInput = false;

	// 이동 차단
	ACharacter* Char = Cast<ACharacter>(this);
	if (Char)
	{
		Char->GetCharacterMovement()->DisableMovement();
	}
}

void AChessPieceBase::Multicast_PlayDeadMontage_Implementation()
{
	if (DeadMontage)
	{
		UAnimInstance* AnimInst = GetMesh()->GetAnimInstance();
		if (AnimInst)
			AnimInst->Montage_Play(DeadMontage);
	}
}

float AChessPieceBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	CurrentHP -= DamageAmount;
	OnHPChanged.Broadcast(CurrentHP);

	if (!DamageCauser)
	{
		UE_LOG(LogTemp, Warning, TEXT("DamageCauser is Null!"));
	}

	if (CurrentHP <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("I'm dead! %s"),*GetName());

		if (ANewChessGameMode* GM =
			GetWorld()->GetAuthGameMode<ANewChessGameMode>())
		{
			AChessPieceBase* WinCH = Cast<AChessPieceBase>(DamageCauser);

			if (WinCH)
			{
				Die();
				FTimerHandle TimerHandle;

				GetWorldTimerManager().SetTimer(TimerHandle, [this, GM, WinCH]()
					{
						GM->SetEndBattle();
					}, 1.0f, false);

			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("remained Hp is %f"), CurrentHP);
	}

	return 0.0f;
}

void AChessPieceBase::PossessCon(AMyChessPlayerController* CP)
{

	UE_LOG(LogTemp, Warning, TEXT("PossessCon %d"), IsLocallyControlled());
	if (IsLocallyControlled())
	{
		if (!CP->PieceIMC)
		{
			UE_LOG(LogTemp, Warning, TEXT("IMC is Null"));
			return;
		}

		if (!LookAction)
		{
			UE_LOG(LogTemp, Warning, TEXT("ClickAction && MoveAction is Null"));
			return;
		}

		if (!CP) return;

		if (ULocalPlayer* LocalPlayer = CP->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
				LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				//Subsystem->ClearAllMappings();
				Subsystem->RemoveMappingContext(CP->CameraIMC);
				Subsystem->AddMappingContext(CP->PieceIMC, 0);
			}
		}

		if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
		{
			EIC->ClearActionBindings();
			EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AChessPieceBase::Look);
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AChessPieceBase::Move);
			EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &AChessPieceBase::DoJumpStart);
			EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &AChessPieceBase::DoJumpEnd);
			EIC->BindAction(AttackAction, ETriggerEvent::Started, this, &AChessPieceBase::Attack);
		}
	}
}

void AChessPieceBase::PawnClientRestart()
{
	Super::PawnClientRestart();

	UE_LOG(LogTemp, Warning, TEXT("PawnClientRestart"));

	AMyChessPlayerController* PC =
		Cast<AMyChessPlayerController>(GetController());

	PossessCon(PC);
}

void AChessPieceBase::ApplyTeamColor()
{
	if (!PieceData || !GetMesh())
	{
		return;
	}

	UMaterialInterface* TeamMaterial = nullptr;

	switch (Team)
	{
	case EChessTeam::White:
		TeamMaterial = WhiteMaterial;
		break;

	case EChessTeam::Black:
		TeamMaterial = BlackMaterial;
		break;

	default:
		return;
	}

	const int32 MaterialCount = GetMesh()->GetNumMaterials();

	for (const int32 SlotIndex : PieceData->TeamMaterialSlots)
	{
		if (SlotIndex >= 0 && SlotIndex < MaterialCount)
		{
			GetMesh()->SetMaterial(SlotIndex, TeamMaterial);
		}
	}
}

void AChessPieceBase::Multicast_ShowBattleWidget_Implementation()
{
	if (!IsLocallyControlled())
	{
		PieceBattleWidgetComp->SetVisibility(true);
	}
}

void AChessPieceBase::Multicast_HideBattleWidget_Implementation()
{
	PieceBattleWidgetComp->SetVisibility(false);
}

void AChessPieceBase::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AChessPieceBase, OwnIndex);
	DOREPLIFETIME(AChessPieceBase, PieceType);
	DOREPLIFETIME(AChessPieceBase, Team);
	DOREPLIFETIME(AChessPieceBase, CurrentHP);
	DOREPLIFETIME(AChessPieceBase, bIsDead);

}

void AChessPieceBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}
