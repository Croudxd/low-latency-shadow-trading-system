# CPP
ENGINE_BIN = ./build/order-book/order-book
KDB_BIN = ./build/kdb/kdb
STRATEGY_BIN = ./build/strategy/strategy
FEEDER_PATH = ./bitfinex-feeder

# PYTHON
STREAMLIT_APP = viewer/main.py

#Q
SCHEMA_PTH = kdb/q/schema.q

.PHONY: all run stop clean

all:
	mkdir -p build
	cd build && cmake .. && make -j

run: all
	q $(SCHEMA_PTH) -p 5001 & echo $$! > .q_db.pid
	$(ENGINE_BIN) & echo $$! > .engine.pid
	$(KDB_BIN) & echo $$! > .kdb_cpp.pid
	$(STRATEGY_BIN) & echo $$! > .strategy.pid
	cd view && uv run python -m streamlit run main.py & echo $$! > ../.streamlit.pid
	cd .. 
	cd $(FEEDER_PATH) && cargo run


stop:
	kill $$(cat .q_db.pid) 2>/dev/null || true
	kill $$(cat .engine.pid) 2>/dev/null || true
	kill $$(cat .kdb_cpp.pid) 2>/dev/null || true
	kill $$(cat .strategy.pid) 2>/dev/null || true
	kill $$(cat .streamlit.pid) 2>/dev/null || true
	rm -f .q_db.pid .engine.pid .kdb_cpp.pid .strategy.pid .streamlit.pid

clean:
	rm -rf build
	cd $(FEEDER_PATH) && cargo clean
