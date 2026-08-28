// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "System/ChessTypes.h"
#include "PiecePrimaryDataAsset.generated.h"

class UAnimeMontage;
class USkeletalMesh;
/**
 * 
 */
UCLASS()
class CHESS_API UPiecePrimaryDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	 UPROPERTY(EditAnywhere)
	 TObjectPtr<UAnimMontage> AttackMontage;

	 UPROPERTY(EditAnywhere)
	 EChessPieceType PieceType;

	 UPROPERTY(EditAnywhere)
	 FChessBattleStat PieceStat;

	 UPROPERTY(EditAnywhere)
	 TObjectPtr<USkeletalMesh> Mesh;

	 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	 TArray<int32> TeamMaterialSlots;
};
