NAME = inception

COMPOSE_FILE = ./srcs/docker-compose.yml

LOGIN = nkawaguc
DATA_PATH = /home/$(LOGIN)/data

all: build up

up:
	@mkdir -p $(DATA_PATH)/wp
	@mkdir -p $(DATA_PATH)/db
	docker compose -f $(COMPOSE_FILE) up -d

build:
	@mkdir -p $(DATA_PATH)/wp
	@mkdir -p $(DATA_PATH)/db
	docker compose -f $(COMPOSE_FILE) up -d --build

clean:
	docker compose -f $(COMPOSE_FILE) down

fclean: clean
	docker system prune -af
	sudo rm -rf $(DATA_PATH)

re: fclean all

.PHONY: all up build clean fclean re