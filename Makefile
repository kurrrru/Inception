NAME = inception

COMPOSE_FILE = ./srcs/docker-compose.yml

LOGIN = kawaguchinagisa
DATA_PATH = /home/$(LOGIN)/data

all: up

create_data_dir:
	@mkdir -p $(DATA_PATH)/wordpress_vol
	@mkdir -p $(DATA_PATH)/mariadb_vol

up: create_data_dir
	cd srcs && docker compose up

build: create_data_dir
	cd srcs && docker compose build

clean:
	cd srcs && docker compose down

fclean: clean
	docker system prune -af
	sudo rm -rf $(DATA_PATH)

re: fclean all

.PHONY: all up build clean fclean re