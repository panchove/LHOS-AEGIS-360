.PHONY: deploy_prod deploy_next prod_hub1 next_hub1 prod_hub2 next_hub2

deploy_prod:
	uv run layrz-app-build platformio platformio

deploy_next:
	uv run layrz-app-build platformio platformio --next

prod_hub1:
	uv run layrz-app-build platformio platformio --flavor production_hub1

next_hub1:
	uv run layrz-app-build platformio platformio --next --flavor production_hub1

prod_hub2:
	uv run layrz-app-build platformio platformio --flavor production_hub2

next_hub2:
	uv run layrz-app-build platformio platformio --next --flavor production_hub2
