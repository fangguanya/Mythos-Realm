#pragma once

#include "Projectile.generated.h"

class AGameCharacter;

UCLASS()
class AProjectile : public AActor
{
	GENERATED_UCLASS_BODY()

protected:

	/* sphere collison */
	UPROPERTY(VisibleAnywhere, Category = Projectile)
	USphereComponent* collisionComp;

	/* projectile movement component */
	UPROPERTY(EditDefaultsOnly, Category = Movement)
	UProjectileMovementComponent* movementComponent;

	/* amount of damage to inflict to the victim */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Damage)
	float damage;

	/* type of damage to do to the victim */
	UPROPERTY(EditDefaultsOnly, Category = Damage)
	TSubclassOf<UDamageType> damageType;

	/* reference to the spawner so we don't collide with them */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile)
	AGameCharacter* projectileSpawner;

	/* reference to the target were homing in on (if any) */
	UPROPERTY(ReplicatedUsing = OnRep_HomingTarget, BlueprintReadWrite, Category=Projectile)
	AGameCharacter* homingTarget;

	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

	/** called when projectile hits something */
	UFUNCTION()
	void OnHit(class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/* called when the homing target is replicated */
	UFUNCTION()
	void OnRep_HomingTarget();

	/*[SERVER] called when the projectile collides for unique effects */
	UFUNCTION(BlueprintImplementableEvent, Category=Projectile)
	void ServerProjectileCollision();

	/*[CLIENT] called when the projectile collides for unique effects */
	UFUNCTION(BlueprintImplementableEvent, Category = Projectile)
	void ClientProjectileCollision();

	/** Returns CollisionComp subobject **/
	FORCEINLINE class USphereComponent* GetCollisionComp() const { return collisionComp; }
	/** Returns ProjectileMovement subobject **/
	FORCEINLINE class UProjectileMovementComponent* GetProjectileMovement() const { return movementComponent; }

public:

	/* initialize and launch the projectile */
	void InitializeProjectile(const FVector& AimDir, float damage, TSubclassOf<UDamageType> projDamage, AGameCharacter* projSpawner = nullptr, AGameCharacter* projTarget = nullptr);

	/* get the projectile owner */
	AGameCharacter* GetProjectileSpawner() const
	{
		return projectileSpawner;
	}
};