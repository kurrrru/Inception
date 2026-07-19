NAME = inception

COMPOSE_FILE = ./srcs/docker-compose.yml

LOGIN = $(shell whoami)
DATA_PATH = /home/$(LOGIN)/data
DOMAIN_NAME = $(LOGIN).42.fr
export DATA_PATH DOMAIN_NAME

all: up hosts

hosts:
	grep -q "$(DOMAIN_NAME)" /etc/hosts || \
		echo "127.0.0.1 $(DOMAIN_NAME)" | sudo tee -a /etc/hosts

create_data_dir:
	mkdir -p $(DATA_PATH)/wordpress_vol
	mkdir -p $(DATA_PATH)/mariadb_vol

up: build
	docker compose -f $(COMPOSE_FILE) up -d

bonus: build_bonus hosts
	USE_REDIS=1 docker compose -f $(COMPOSE_FILE) --profile bonus up -d

down:
	docker compose -f $(COMPOSE_FILE) --profile bonus down

build: create_data_dir
	docker compose -f $(COMPOSE_FILE) build

build_bonus: create_data_dir
	docker compose -f $(COMPOSE_FILE) --profile bonus build

clean:
	docker compose -f $(COMPOSE_FILE) --profile bonus down --rmi all --volumes

fclean: clean
	sudo rm -rf $(DATA_PATH)

re: fclean all

.PHONY: all up down build build_bonus clean fclean re create_data_dir hosts bonus
