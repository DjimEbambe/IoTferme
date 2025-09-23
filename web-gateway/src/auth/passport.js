import passport from 'passport';
import { Strategy as LocalStrategy } from 'passport-local';
import bcrypt from 'bcrypt';
import { UserModel } from '../dao/models/common.js';
import { env } from '../config/env.js';

const strategy = new LocalStrategy(
  { usernameField: 'identifier', passwordField: 'password', passReqToCallback: true },
  async (req, identifier, password, done) => {
    try {
      const loginId = (identifier || '').trim();
      if (!loginId) {
        return done(null, false, { message: 'Identifiant requis' });
      }
      const isEmail = loginId.includes('@');
      const query = isEmail
        ? { email: loginId.toLowerCase(), active: true }
        : { phone: loginId.replace(/\D/g, ''), active: true };
      const user = await UserModel.findOne(query).lean(false);
      if (!user) {
        return done(null, false, { message: 'Invalid credentials' });
      }
      const ok = await bcrypt.compare(password, user.passwordHash);
      if (!ok) {
        return done(null, false, { message: 'Invalid credentials' });
      }
      if (!user.roles || user.roles.length === 0) {
        user.roles = [env.rbacDefaultRole];
      }
      return done(null, {
        id: user._id.toString(),
        email: user.email,
        phone: user.phone,
        roles: user.roles,
        scopes: user.scopes || {},
        displayName: user.displayName,
      });
    } catch (err) {
      return done(err);
    }
  }
);

passport.use(strategy);

passport.serializeUser((user, done) => {
  done(null, {
    id: user.id,
    roles: user.roles,
    scopes: user.scopes,
    email: user.email,
    phone: user.phone,
  });
});

passport.deserializeUser(async ({ id }, done) => {
  try {
    const user = await UserModel.findById(id).lean();
    if (!user) {
      return done(null, false);
    }
    done(null, {
      id: user._id.toString(),
      email: user.email,
      phone: user.phone,
      roles: user.roles,
      scopes: user.scopes || {},
      displayName: user.displayName,
    });
  } catch (error) {
    done(error);
  }
});

export { passport };
