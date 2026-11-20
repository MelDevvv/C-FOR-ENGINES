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
class UMainUIWidget;
class UBoostHUDWidget;
class UShieldComponent;
class ARocket;
class UMaterialInterface;
class UMaterialInstanceDynamic;
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

   /** Called by the native PlayerController when Left Shift is pressed/released. */
   void SetBoostInput(bool bPressed);
   void ActivateShieldAbility();
   void FireSecondaryRocket();
   void ActivateLightningBlast();
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank|Movement",
       meta = (ClampMin = "0.0"))
   float TurnSpeed = 100.0f;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tank|Movement", meta = (ClampMin = "0.0"))
   float NormalMoveSpeed = 400.0f;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tank|Boost", meta = (ClampMin = "0.0"))
   float BoostMoveSpeed = 1000.0f;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tank|Boost", meta = (ClampMin = "1.0"))
   float MaximumBoostEnergy = 100.0f;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tank|Boost", meta = (ClampMin = "0.0"))
   float BoostDrainPerSecond = 35.0f;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tank|Boost", meta = (ClampMin = "0.0"))
   float BoostRechargePerSecond = 22.0f;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tank|Boost", meta = (ClampMin = "0.0"))
   float BoostCameraFOV = 102.0f;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tank|Boost", meta = (ClampMin = "0.1"))
   float BoostFOVInterpSpeed = 5.0f;
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank|Weapons",
       meta = (ClampMin = "0.05"))
   float FireRateSeconds = 0.5f;
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
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank|Abilities")
   TObjectPtr<UShieldComponent> ShieldComp;

   // Visual-only shield bubble. Collision is disabled so it cannot affect gameplay.
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank|Abilities|Shield Visual")
   TObjectPtr<UStaticMeshComponent> ShieldBubble;

   // Assign a translucent/emissive shield material in the Tank Blueprint.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tank|Abilities|Shield Visual")
   TObjectPtr<UMaterialInterface> ShieldBubbleMaterial;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tank|Weapons")
   TSubclassOf<AActor> ProjectileClass;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tank|Weapons")
   TObjectPtr<UParticleSystem> ShotCaseEffect;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tank|Abilities|Lightning", meta = (ClampMin = "0.0"))
   float LightningBlastRadius = 1000.0f;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tank|Abilities|Lightning", meta = (ClampMin = "0.0"))
   float LightningBlastDamage = 50.0f;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tank|Abilities|Lightning", meta = (ClampMin = "0.0"))
   float LightningBlastCooldown = 30.0f;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tank|Abilities|Lightning")
   TObjectPtr<UParticleSystem> LightningBlastEffect;
   // Widget class loaded from WBP_MainUI.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tank|UI")
   TSubclassOf<UMainUIWidget> TankHUDClass;
private:
   UPROPERTY(Transient)
   TObjectPtr<UMainUIWidget> TankHUD;

   UPROPERTY(Transient)
   TObjectPtr<UBoostHUDWidget> BoostHUD;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tank|Weapons", meta=(AllowPrivateAccess="true"))
   TSubclassOf<ARocket> SecondaryRocketClass;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tank|Weapons", meta=(ClampMin="0.1", AllowPrivateAccess="true"))
   float SecondaryRocketCooldown = 3.0f;

   float SecondaryRocketCooldownRemaining = 0.0f;
   float LightningBlastCooldownRemaining = 0.0f;
   FTransform RespawnTransform;
   FTimerHandle FireRateTimerHandle;
   FTimerHandle RespawnTimerHandle;
   FTimerHandle SecondaryRocketTimerHandle;
   int32 NextGunIndex = 0;
   bool bCanFire = true;
   bool bIsRespawning = false;
   bool bBoostInputHeld = false;
   bool bIsBoosting = false;
   bool bBoostLockedUntilRelease = false;
   float CurrentBoostEnergy = 100.0f;
   float NormalCameraFOV = 90.0f;
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
   void CreateBoostHUD();
   void UpdateBoost(float DeltaTime);
   void UpdateShieldVisual();
   void StopBoost();
   void ResetSecondaryRocketCooldown();
};