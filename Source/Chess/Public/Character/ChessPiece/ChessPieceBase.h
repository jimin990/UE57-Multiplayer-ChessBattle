// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "System/ChessTypes.h"
#include "InputActionValue.h"
#include "ChessPieceBase.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UInputMappingContext;
class UInputAction;
class UAnimMontage;
class AMyChessPlayerController;
class UWidgetComponent;
class UPieceBattleWidget;
class UPiecePrimaryDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnHPChanged,
	float, NewHP
);

UCLASS()
class CHESS_API AChessPieceBase : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AChessPieceBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	//현재 자신이 있는 체스판 위치
	UPROPERTY(Replicated)
	int32 OwnIndex;

	UPROPERTY(Replicated)
	EChessPieceType PieceType;

	UPROPERTY(ReplicatedUsing = OnRep_Team)
	EChessTeam Team;
	
	UFUNCTION()
	void OnRep_Team();
// 전투 시스템
public:
	UPROPERTY(EditAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArmComponent;

	UPROPERTY(EditAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> CameraComponent;
// 전투 스텟
public:
	UPROPERTY(EditAnywhere)
	FChessBattleStat Stat;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHP)
	float CurrentHP;

	UPROPERTY(EditAnywhere, Category = "Data")
	TObjectPtr<UPiecePrimaryDataAsset> PieceData;

	UFUNCTION()
	void OnRep_CurrentHP();

public:
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> PieceIMC;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> AttackAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UAnimMontage> DeadMontage;

	void Look(const FInputActionValue& Value);

	void Move(const FInputActionValue& Value);

	void DoJumpStart();

	void DoJumpEnd();
	
	// 입력
	void Attack();

	UFUNCTION(Server, Reliable)
	void Server_Attack();
	void Server_Attack_Implementation();

	// ===== 콤보 시작 =====
	void StartCombo();

	// ===== 멀티캐스트 애니메이션 =====
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayComboMontage(int32 ComboIndex);

	// ===== 콤보 체크 (Anim Notify) =====
	UFUNCTION(BlueprintCallable)
	void AnimNotify_ComboCheck();

	// ===== 콤보 종료 (Anim Notify) =====
	UFUNCTION(BlueprintCallable)
	void AnimNotify_EndCombo();

	// ===== 데미지 처리 =====
	UFUNCTION(BlueprintCallable)
	void AnimNotify_AttackHit();

	// AChessPieceBase.h
	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	int32 MaxCombo = 4;

	UPROPERTY(VisibleAnywhere, Replicated, Category = "Combo")
	bool bIsAttacking = false;

	UPROPERTY(VisibleAnywhere, Replicated, Category = "Combo")
	bool bComboInput = false;

	UPROPERTY(VisibleAnywhere, Replicated, Category = "Combo")
	int32 PieceComboIndex = 0;

public:
	// 서버에서 호출하는 사망 함수
	UFUNCTION()
	void Die();

	void OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// 모든 클라이언트에서 죽음 몽타주 재생
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayDeathMontage();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Dead();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayDeadMontage();

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		class AActor* DamageCauser
	) override;

	void PossessCon(AMyChessPlayerController* CP);

	virtual void PawnClientRestart() override;

	//UI
public:
	UPROPERTY(EditAnywhere, Category = "Widget")
	TObjectPtr<UWidgetComponent> PieceBattleWidgetComp;

	UPROPERTY(EditAnywhere, Category = "Widget")
	TSubclassOf<UPieceBattleWidget> PieceBattleWidgetClass;

	UPROPERTY(BlueprintAssignable)
	FOnHPChanged OnHPChanged;

	// 배틀 위젯 표시

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ShowBattleWidget();
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_HideBattleWidget();

public:
	// 팀 색상 적용 함수
	void ApplyTeamColor();

	UPROPERTY(EditAnywhere, Category = "Material")
	TObjectPtr<UMaterialInstance> WhiteMaterial;

	UPROPERTY(EditAnywhere, Category = "Material")
	TObjectPtr<UMaterialInstance> BlackMaterial;

	virtual void PostInitializeComponents() override;

public:
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "State")
	bool bIsDead = false;

};
