SOURCE1 = index.html
SOURCE2 = darktheme.css
TARGET1 = html
TARGET2 = css

all:
	@echo "Use a specific target: $(TARGET1) for $(SOURCE1) and $(TARGET2) for $(SOURCE2)"

$(TARGET1):
	@nc 10.0.0.42 1337 < $(SOURCE1)

$(TARGET2):
	@nc 10.0.0.42 1337 < $(SOURCE2)