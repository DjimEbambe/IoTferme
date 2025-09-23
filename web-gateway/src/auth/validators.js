import Joi from 'joi';

export const signupSchema = Joi.object({
  email: Joi.string().email({ tlds: false }).allow('', null),
  phone: Joi.string()
    .pattern(/[0-9()+\-\s]{6,}/)
    .allow('', null),
  password: Joi.string()
    .min(8)
    .max(72)
    .pattern(/^(?=.*[a-z])(?=.*[A-Z])(?=.*\d).+$/)
    .message('Le mot de passe doit contenir majuscules, minuscules et chiffres.')
    .required(),
  confirmPassword: Joi.string()
    .valid(Joi.ref('password'))
    .required()
    .messages({
      'any.only': 'Les mots de passe ne correspondent pas.',
    }),
  displayName: Joi.string().max(120).allow('', null),
})
  .custom((value, helpers) => {
    if (!value.email && !value.phone) {
      return helpers.error('any.custom', 'Un email ou numéro est requis');
    }
    return value;
  })
  .messages({
    'any.custom': '{{#label}}',
  });

export const sanitizeSignupPayload = (payload) => {
  const { confirmPassword, ...rest } = payload ?? {};
  const data = { ...rest };
  if (data.email) {
    data.email = data.email.trim().toLowerCase();
  }
  if (data.phone) {
    data.phone = data.phone.replace(/\D/g, '');
  }
  return data;
};
