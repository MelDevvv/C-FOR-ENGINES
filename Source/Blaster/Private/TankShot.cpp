#include "TankShot.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "TimerManager.h"
#include "Engine/World.h"
ATankShot::ATankShot()
{
   PrimaryActorTick.bCanEverTick = false;
   Sphere = CreateDefaultSubobject<USphereComponent>(
       TEXT("Sphere")
   );
   SetRootComponent(Sphere);
   Sphere->InitSphereRadius(5.0f);
   Sphere->SetCollisionEnabled(
       ECollisionEnabled::QueryOnly
   );
   Sphere->SetCollisionResponseToAllChannels(
       ECR_Overlap
   );
   Sphere->SetGenerateOverlapEvents(true);
   StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(
       TEXT("StaticMesh")
   );
   StaticMesh->SetupAttachment(Sphere);
   StaticMesh->SetCollisionEnabled(
       ECollisionEnabled::NoCollision
   );
   StaticMesh->SetRelativeScale3D(
       FVector(5.0f)
   );
   ProjectileMovement =
       CreateDefaultSubobject<UProjectileMovementComponent>(
           TEXT("ProjectileMovement")
       );
   ProjectileMovement->UpdatedComponent = Sphere;
   ProjectileMovement->InitialSpeed = 500.0f;
   ProjectileMovement->MaxSpeed = 500.0f;
   ProjectileMovement->ProjectileGravityScale = 0.0f;
   ProjectileMovement->bInitialVelocityInLocalSpace = true;
   ProjectileMovement->bRotationFollowsVelocity = false;
   ProjectileMovement->bShouldBounce = false;
   InitialLifeSpan = 0.0f;
}
void ATankShot::BeginPlay()
{
   Super::BeginPlay();
   if (Sphere)
   {
       Sphere->OnComponentBeginOverlap.AddUniqueDynamic(
           this,
&ATankShot::OnSphereBeginOverlap
       );
   }
   if (GetWorld() && KillshotDelay > 0.0f)
   {
       GetWorld()->GetTimerManager().SetTimer(
           KillshotTimerHandle,
           this,
&ATankShot::Killshot,
           KillshotDelay,
           false
       );
   }
}
void ATankShot::OnSphereBeginOverlap(
   UPrimitiveComponent* OverlappedComponent,
   AActor* OtherActor,
   UPrimitiveComponent* OtherComponent,
   int32 OtherBodyIndex,
   bool bFromSweep,
   const FHitResult& SweepResult
)
{
   if (bHasImpacted || !IsValid(OtherActor))
   {
       return;
   }
   // Ignore the tank that fired this projectile.
   if (OtherActor == GetOwner() ||
       OtherActor == GetInstigator())
   {
       return;
   }
   bHasImpacted = true;
   UGameplayStatics::ApplyDamage(
       OtherActor,
       Damage,
       GetInstigatorController(),
       this,
       nullptr
   );
   if (ImpactEffect && GetWorld())
   {
       UGameplayStatics::SpawnEmitterAtLocation(
           GetWorld(),
           ImpactEffect,
           GetActorLocation(),
           GetActorRotation(),
           FVector(0.5f),
           true
       );
   }
   if (GetWorld())
   {
       GetWorld()->GetTimerManager().ClearTimer(
           KillshotTimerHandle
       );
   }
   Destroy();
}
void ATankShot::Killshot()
{
   if (!IsValid(this))
   {
       return;
   }
   if (KillshotEffect && GetWorld())
   {
       UGameplayStatics::SpawnEmitterAtLocation(
           GetWorld(),
           KillshotEffect,
           GetActorTransform(),
           true
       );
   }
   Destroy();
}