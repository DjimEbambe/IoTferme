.PHONY: install bootstrap up down seed test lint diagram

install:
	npm install --prefix web-gateway
	npm install --prefix iot-streamer
	python3 -m venv ml-engine/venv
	./ml-engine/venv/bin/pip install --upgrade pip
	./ml-engine/venv/bin/pip install -r ml-engine/requirements.txt
	npm run build:client --prefix web-gateway

bootstrap:
	sudo ./scripts/bootstrap_vps.sh

seed:
	cd web-gateway && npm run seed

up:
	npm run watch:client --prefix web-gateway &
	npm run dev --prefix web-gateway &
	npm run dev --prefix iot-streamer &
	cd ml-engine && ./venv/bin/uvicorn app.main:app --reload --host 127.0.0.1 --port 8002

down:
	pkill -f "webpack --watch" || true
	pkill -f "node src/server.js" || true
	pkill -f "node src/index.js" || true
	pkill -f "uvicorn app.main:app" || true

lint:
	npm run lint --prefix web-gateway
	npm run lint --prefix iot-streamer
	./ml-engine/venv/bin/ruff ml-engine/app

test:
	npm test --prefix web-gateway
	npm test --prefix iot-streamer
	./ml-engine/venv/bin/pytest -q

diagram:
	@echo "Installer PlantUML et exécuter : plantuml plantuml/container.puml"
