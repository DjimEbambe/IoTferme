from app.models.optimize import OptimizeRationRequest, Ingredient, RationConstraints
from app.services.optimize_ration import optimize_ration


def test_optimize_ration():
    req = OptimizeRationRequest(
        ingredients=[
            Ingredient(name='Maïs', cost_per_kg=0.4, protein=8, energy=12, fiber=2, max_ratio=0.7),
            Ingredient(name='Soja', cost_per_kg=0.6, protein=44, energy=10, fiber=6, max_ratio=0.4),
        ],
        constraints=RationConstraints(total_mass=100, min_protein=20, min_energy=10, max_fiber=5),
    )
    res = optimize_ration(req)
    assert res.cost_total > 0
    assert abs(sum(a.kg for a in res.allocations) - 100) < 1e-3
