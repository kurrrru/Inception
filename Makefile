NAME = inception

COMPOSE_FILE = ./srcs/docker-compose.yml

LOGIN = $(shell whoami)
DATA_PATH = /home/$(LOGIN)/data
export DATA_PATH

all: up

create_data_dir:
	mkdir -p $(DATA_PATH)/wordpress_vol
	mkdir -p $(DATA_PATH)/mariadb_vol

up: build
	docker compose -f $(COMPOSE_FILE) up -d

build: create_data_dir
	docker compose -f $(COMPOSE_FILE) build

clean:
	docker compose -f $(COMPOSE_FILE) down --rmi all --volumes

fclean: clean
	sudo rm -rf $(DATA_PATH)

re: fclean all

.PHONY: all up build clean fclean re create_data_dir
