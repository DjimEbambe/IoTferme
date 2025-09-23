PYTHON?=python3
EDGE_DIR=edge-agent
ABS_EDGE_DIR:=$(abspath $(EDGE_DIR))
VENV?=$(EDGE_DIR)/.venv
ABS_VENV:=$(abspath $(VENV))
UVICORN?=$(ABS_VENV)/bin/uvicorn

SYSTEMD_UNIT=edge-agent.service
SYSTEMD_DIR=/etc/systemd/system
ENV_FILE=/etc/edge-agent.env

SERVICE_TEMPLATE=$(EDGE_DIR)/systemd/edge-agent.service.tmpl
BUILD_DIR:=$(abspath build)
GENERATED_SERVICE=$(BUILD_DIR)/edge-agent.service

SERVICE_USER?=$(shell id -un)
SERVICE_GROUP?=$(shell id -gn)
DATA_DIR?=/var/lib/edge-agent
LOG_DIR?=/var/log/edge-agent

PYTHON_BIN=$(ABS_VENV)/bin/python

.PHONY: help install enable start stop status logs seed enroll lint test flash-gateway flash-sensor flash-pzem

help:
	@echo 'Targets:'
	@echo '  make install       # Installe dépendances edge-agent sur Pi'
	@echo '  make enable        # Génère et installe le service systemd/udev'
	@echo '  make start         # Démarre edge-agent via systemd'
	@echo '  make stop          # Stoppe edge-agent via systemd'
	@echo '  make status        # Status systemd'
	@echo '  make logs          # Journaux journald'
	@echo '  make seed          # Remplit SQLite avec données de démo'
	@echo '  make enroll        # Provisioning devices (tools/enroll_devices.py)'
	@echo '  make lint          # Formate/force lint edge-agent'
	@echo '  make test          # Exécute pytest edge-agent'
	@echo '  make flash-*       # Proxy vers tools/fw_flash.sh'

install: export SERVICE_USER=$(SERVICE_USER)
install: export SERVICE_GROUP=$(SERVICE_GROUP)
install: export DATA_DIR=$(DATA_DIR)
install: export LOG_DIR=$(LOG_DIR)
install:
	bash scripts/setup_edge.sh

$(GENERATED_SERVICE): $(SERVICE_TEMPLATE) Makefile
	@mkdir -p $(BUILD_DIR)
	@sed \
		-e 's|@SERVICE_USER@|$(SERVICE_USER)|g' \
		-e 's|@SERVICE_GROUP@|$(SERVICE_GROUP)|g' \
		-e 's|@EDGE_DIR@|$(ABS_EDGE_DIR)|g' \
		-e 's|@ENV_FILE@|$(ENV_FILE)|g' \
		-e 's|@UVICORN_CMD@|$(UVICORN)|g' \
		< $(SERVICE_TEMPLATE) > $(GENERATED_SERVICE)

enable: $(GENERATED_SERVICE)
	sudo install -m 644 $(GENERATED_SERVICE) $(SYSTEMD_DIR)/$(SYSTEMD_UNIT)
	@if [ ! -f $(ENV_FILE) ]; then \
		sudo install -m 640 -o $(SERVICE_USER) -g $(SERVICE_GROUP) $(EDGE_DIR)/.env.example $(ENV_FILE); \
	else \
		echo '$(ENV_FILE) already exists; leaving in place.'; \
	fi
	sudo install -m 644 $(EDGE_DIR)/udev/99-edge-gateway.rules /etc/udev/rules.d/99-edge-gateway.rules
	sudo mkdir -p $(DATA_DIR)
	sudo chown $(SERVICE_USER):$(SERVICE_GROUP) $(DATA_DIR)
	sudo mkdir -p $(LOG_DIR)
	sudo touch $(LOG_DIR)/edge-agent.log
	sudo chown $(SERVICE_USER):$(SERVICE_GROUP) $(LOG_DIR) $(LOG_DIR)/edge-agent.log
	sudo systemctl daemon-reload
	sudo udevadm control --reload-rules
	sudo udevadm trigger

start:
	sudo systemctl start $(SYSTEMD_UNIT)

stop:
	sudo systemctl stop $(SYSTEMD_UNIT)

status:
	sudo systemctl status $(SYSTEMD_UNIT)

logs:
	sudo journalctl -u $(SYSTEMD_UNIT) -f

seed:
	@if [ ! -x $(PYTHON_BIN) ]; then \
		echo "Virtualenv not found. Run 'make install' first."; \
		exit 1; \
	fi
	$(PYTHON_BIN) $(EDGE_DIR)/scripts/seed_demo.py

enroll:
	@if [ ! -x $(PYTHON_BIN) ]; then \
		echo "Virtualenv not found. Run 'make install' first."; \
		exit 1; \
	fi
	$(PYTHON_BIN) tools/enroll_devices.py

lint:
	@if [ ! -x $(ABS_VENV)/bin/ruff ]; then \
		echo "Virtualenv not found. Run 'make install' first."; \
		exit 1; \
	fi
	$(ABS_VENV)/bin/ruff format $(EDGE_DIR)/src
	$(ABS_VENV)/bin/ruff check $(EDGE_DIR)/src

test:
	@if [ ! -x $(ABS_VENV)/bin/pytest ]; then \
		echo "Virtualenv not found. Run 'make install' first."; \
		exit 1; \
	fi
	$(ABS_VENV)/bin/pytest $(EDGE_DIR)/tests

flash-gateway:
	tools/fw_flash.sh gateway

flash-sensor:
	tools/fw_flash.sh sensor

flash-pzem:
	tools/fw_flash.sh pzem
