#include "BaseGameRule.h"

UBaseGameRule::UBaseGameRule()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBaseGameRule::InitRule_Implementation(int32 InRuleID)
{
	RuleID = InRuleID;
	Completed = false;
}

void UBaseGameRule::CompleteRule_Implementation(bool bSuccessful)
{
	if (Completed)
	{
		return;
	}

	Completed = true;

	OnRuleUpdated.Broadcast(RuleID, Completed);
	OnRuleComplete.Broadcast(RuleID, bSuccessful);
}

void UBaseGameRule::UpdateRuleConditions_Implementation()
{
	OnRuleUpdated.Broadcast(RuleID, Completed);
}