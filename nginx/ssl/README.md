# Certificats TLS

Déposez vos certificats dans `/etc/nginx/ssl/farmstack/` :

- `fullchain.crt` : certificat complet + chaîne intermédiaire
- `privkey.key` : clé privée (droits 600)

Ensuite, activez le bloc HTTPS :

```bash
sudo ln -s /etc/nginx/sites-available/farmstack.conf /etc/nginx/sites-enabled/
sudo nginx -t && sudo systemctl reload nginx
```

> Aucun Certbot : les certificats sont gérés manuellement (AC interne, Cloudflare Origin, etc.).
