SHELL := /bin/bash

-include config.env

PS5_HOST ?=
PS5_PORT ?=
PS5_WEB_PORT ?=
NASGAMES_PORT ?= 8098
export PS5_HOST PS5_PORT PS5_WEB_PORT NASGAMES_PORT

RUN := bash scripts/run.sh

.PHONY: bootstrap env check hello deploy browser deploy-browser nasgames deploy-nasgames release-nasgames release-nasgames-clean pm-upload-nasgames pm-autoload-nasgames nasgames-health clean

bootstrap:
	@bash scripts/bootstrap.sh

env:
	@bash -c 'source scripts/env.sh; printf "PS5_PAYLOAD_SDK=%s\nPS5_HOST=%s\nPS5_PORT=%s\n" "$$PS5_PAYLOAD_SDK" "$$PS5_HOST" "$$PS5_PORT"'

check:
	@bash scripts/check.sh

hello:
	@$(RUN) $(MAKE) -C apps/hello

deploy: hello
	@bash scripts/deploy.sh apps/hello/hello.elf

browser:
	@$(RUN) $(MAKE) -C apps/browser

deploy-browser: browser
	@bash scripts/deploy.sh apps/browser/browser.elf

nasgames:
	@$(RUN) $(MAKE) -C apps/nasgames

deploy-nasgames: nasgames
	@bash scripts/deploy.sh apps/nasgames/nasgames.elf
	@printf 'Waiting for NAS Games on http://%s:%s ...\n' "$${PS5_HOST}" "$(NASGAMES_PORT)"
	@for i in $$(seq 1 20); do \
		if curl -fsS --max-time 2 "http://$${PS5_HOST}:$(NASGAMES_PORT)/api/health" >/dev/null; then \
			printf 'NAS Games is ready: http://%s:%s\n' "$${PS5_HOST}" "$(NASGAMES_PORT)"; \
			exit 0; \
		fi; \
		sleep 1; \
	done; \
	printf 'Payload was sent, but the HTTP port did not answer yet.\n' >&2; \
	exit 1

release-nasgames: nasgames
	@bash scripts/release-managed.sh

release-nasgames-clean: nasgames
	@$(RUN) $(MAKE) -C apps/nasgames release-prepare
	@bash scripts/release-ps5.sh

pm-upload-nasgames: nasgames
	@bash scripts/pm-upload.sh apps/nasgames/nasgames.elf

pm-autoload-nasgames: nasgames
	@bash scripts/pm-upload.sh apps/nasgames/nasgames.elf --autoload

nasgames-health:
	@curl -fsS --max-time 4 "http://$${PS5_HOST}:$(NASGAMES_PORT)/api/health" && printf '\n'

clean:
	@$(RUN) $(MAKE) -C apps/hello clean
	@$(RUN) $(MAKE) -C apps/browser clean
	@$(RUN) $(MAKE) -C apps/nasgames clean
