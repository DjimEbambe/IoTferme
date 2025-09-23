import { rolePermissions } from '../rbac/roles.js';
import { permissions } from '../rbac/permissions.js';
import { scopeTypes, normalizeScopes } from '../rbac/scopes.js';

const hasPermission = (user, permission) => {
  if (!user) return false;
  const perms = new Set();
  (user.roles || []).forEach((role) => {
    (rolePermissions[role] || []).forEach((perm) => perms.add(perm));
  });
  return perms.has(permission);
};

const hasScope = (user, scope) => {
  if (!scope) return true;
  const userScopes = normalizeScopes(user?.scopes);
  return scopeTypes.every((type) => {
    if (!scope[type]) return true;
    const scopeList = Array.isArray(scope[type]) ? scope[type] : [scope[type]];
    return scopeList.every((value) => userScopes[type]?.includes(value));
  });
};

export const rbac = {
  permissions,
  hasPermission,
  hasScope,
};
