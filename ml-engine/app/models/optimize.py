from typing import List
from pydantic import BaseModel, Field


class Ingredient(BaseModel):
    name: str
    cost_per_kg: float = Field(..., gt=0)
    protein: float = Field(..., ge=0)
    energy: float = Field(..., ge=0)
    fiber: float = Field(0, ge=0)
    max_ratio: float = Field(1.0, ge=0, le=1.0)


class RationConstraints(BaseModel):
    total_mass: float = Field(100, gt=0)
    min_protein: float = Field(18, ge=0)
    min_energy: float = Field(11, ge=0)
    max_fiber: float = Field(6, ge=0)


class OptimizeRationRequest(BaseModel):
    ingredients: List[Ingredient]
    constraints: RationConstraints


class RationAllocation(BaseModel):
    name: str
    kg: float
    ratio: float


class OptimizeRationResponse(BaseModel):
    allocations: List[RationAllocation]
    cost_total: float
