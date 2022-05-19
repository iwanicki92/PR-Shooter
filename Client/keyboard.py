import pygame

pygame.init()
display_width = 500
display_height = 500

result = pygame.display.set_mode((display_width, display_height))

pygame.display.set_caption('PR-Shooter')
x = 80
y = 80
width = 20
height = 20
value = 3

run = True
while run:
    pygame.time.delay(100)

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            run = False

    keys = pygame.key.get_pressed()

    if keys[pygame.K_w]:
        y -= value
    if keys[pygame.K_s]:
        y += value
    if keys[pygame.K_a]:
        x -= value
    if keys[pygame.K_d]:
        x += value
    pygame.draw.rect(result, (0, 0, 255), (x, y, width, height))
    pygame.display.update()

    result.fill((0, 0, 0))
pygame.quit()








