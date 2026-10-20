#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Inputable.h"
#include "Tank.generated.h"
class UArrowComponent;
class UStaticMeshComponent;
class UParticleSystemComponent;
class UParticleSystem;
class USpringArmComponent;
class UCameraComponent;
class UHealthComponent;
class UUserWidget;
UCLASS(Abstract)
class BLASTER_API ATank : public ACharacter, public IInputable
{
   GENERATED_BODY()
public:
   ATank();
   virtual void Tick(float DeltaTime) override;
   virtual void IMove_Implementation(float InputValue) override;
   virtual void ITurn_Implementation(float InputValue) override;
   virtual void IPrimaryAction_Implementation(bool bPressed) override;
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank|Movement",
       meta = (ClampMin = "0.0"))
   float TurnSpeed = 100.0f;
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank|Weapons",
       meta = (ClampMin = "0.05"))
   float FireRateSeconds = 0.5f;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tank|UI")
   TSubclassOf<UUserWidget> TankHUDClass;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tank|Respawn",
       meta = (ClampMin = "0.1"))
   float RespawnTime = 3.0f;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tank|Effects")
   TObjectPtr<UParticleSystem> DeathEffect;
protected:
   virtual void BeginPlay() override;
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
   TObjectPtr<UArrowComponent> RotTracks;
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
   TObjectPtr<UStaticMeshComponent> Plane;
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
   TObjectPtr<UStaticMeshComponent> Track01;
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
   TObjectPtr<UParticleSystemComponent> Treads;
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
   TObjectPtr<UStaticMeshComponent> Track02;
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
   TObjectPtr<UParticleSystemComponent> Treads1;
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
   TObjectPtr<USpringArmComponent> SpringArm;
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
   TObjectPtr<UCameraComponent> Camera;
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
   TObjectPtr<UStaticMeshComponent> TCore01;
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
   TObjectPtr<UStaticMeshComponent> TGun01;
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
   TObjectPtr<UArrowComponent> W1;
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
   TObjectPtr<UArrowComponent> W2;
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
   TObjectPtr<UStaticMeshComponent> TGun02;
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
   TObjectPtr<UArrowComponent> W3;
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
   TObjectPtr<UArrowComponent> W4;
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
   TObjectPtr<UHealthComponent> HealthComp;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tank|Weapons")
   TSubclassOf<AActor> ProjectileClass;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tank|Weapons")
   TObjectPtr<UParticleSystem> ShotCaseEffect;
private:
   UPROPERTY(Transient)
   TObjectPtr<UUserWidget> TankHUD;
   FTransform RespawnTransform;
   FTimerHandle FireRateTimerHandle;
   FTimerHandle RespawnTimerHandle;
   int32 NextGunIndex = 0;
   bool bCanFire = true;
   bool bIsRespawning = false;
   float Gun1ShudderTime = 0.0f;
   float Gun2ShudderTime = 0.0f;
   bool bGun1ShudderActive = false;
   bool bGun2ShudderActive = false;
   FVector Gun1RestLocation = FVector::ZeroVector;
   FVector Gun2RestLocation = FVector::ZeroVector;
   void FireProjectile(UArrowComponent* SpawnPoint, bool bFirstGun);
   void StartGunShudder(bool bFirstGun);
   void UpdateGunShudders(float DeltaTime);
   void ResetFireCooldown();
   UFUNCTION()
   void HandleTankDeath();
   void RespawnTank();
   void CreateTankHUD();
};