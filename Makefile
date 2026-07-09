NAME = inception

COMPOSE_FILE = ./srcs/docker-compose.yml

LOGIN = $(shell whoami)
DATA_PATH = /home/$(LOGIN)/data

all: up

create_data_dir:
	@mkdir -p $(DATA_PATH)/wordpress_vol
	@mkdir -p $(DATA_PATH)/mariadb_vol

up: build
	cd srcs && docker compose up -d

build: create_data_dir
	cd srcs && docker compose build

clean:
	cd srcs && docker compose down --rmi all --volumes

fclean: clean
	sudo rm -rf $(DATA_PATH)

re: fclean all

.PHONY: all up build clean fclean re create_data_dir
