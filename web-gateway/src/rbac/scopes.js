export const scopeTypes = ['site', 'building', 'zone', 'lot'];

export const scopeDefaults = {
  site: ['SITE:KIN-GOLIATH'],
  building: ['BLDG:A', 'BLDG:B'],
};

export const normalizeScopes = (scopes = {}) =>
  scopeTypes.reduce((acc, key) => {
    acc[key] = Array.isArray(scopes[key]) && scopes[key].length > 0 ? scopes[key] : scopeDefaults[key] || [];
    return acc;
  }, {});
