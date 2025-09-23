import cvxpy as cp
import numpy as np
from ..models.optimize import OptimizeRationRequest, RationAllocation, OptimizeRationResponse


def optimize_ration(req: OptimizeRationRequest) -> OptimizeRationResponse:
    ingredients = req.ingredients
    n = len(ingredients)
    if n == 0:
        raise ValueError('No ingredients provided')

    x = cp.Variable(n)
    costs = np.array([ing.cost_per_kg for ing in ingredients])
    protein = np.array([ing.protein for ing in ingredients])
    energy = np.array([ing.energy for ing in ingredients])
    fiber = np.array([ing.fiber for ing in ingredients])

    constraints = [cp.sum(x) == req.constraints.total_mass]
    constraints.append(protein @ x >= req.constraints.min_protein * req.constraints.total_mass / 100)
    constraints.append(energy @ x >= req.constraints.min_energy * req.constraints.total_mass / 100)
    constraints.append(fiber @ x <= req.constraints.max_fiber * req.constraints.total_mass / 100)
    constraints.extend([x >= 0])
    for idx, ing in enumerate(ingredients):
        constraints.append(x[idx] <= ing.max_ratio * req.constraints.total_mass)

    problem = cp.Problem(cp.Minimize(costs @ x), constraints)
    problem.solve(solver=cp.ECOS, warm_start=True)

    if problem.status not in (cp.OPTIMAL, cp.OPTIMAL_INACCURATE):
        raise ValueError('Unable to find optimal ration')

    allocations = []
    total_mass = req.constraints.total_mass
    for idx, ing in enumerate(ingredients):
        kg = float(x.value[idx])
        allocations.append(
            RationAllocation(name=ing.name, kg=round(kg, 3), ratio=round(kg / total_mass, 4))
        )
    return OptimizeRationResponse(allocations=allocations, cost_total=float(problem.value))
