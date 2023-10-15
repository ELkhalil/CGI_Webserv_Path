#!/usr/bin/env python

# Set the content type to HTML
print("Content-Type: text/html\n")

# HTML code to create a canvas and animate multiple bouncing balls
print("""
<!DOCTYPE html>
<html>
<head>
    <title>CGI Bouncing Balls</title>
</head>
<body>
    <h1>Python CGI Bouncing Balls</h1>
    <canvas id="myCanvas" width="600" height="400" style="border:1px solid #000;"></canvas>
    <script>
        var canvas = document.getElementById("myCanvas");
        var context = canvas.getContext("2d");
        var balls = [];

        // Ball class to represent each bouncing ball
        class Ball {
            constructor(x, y, radius, dx, dy, color) {
                this.x = x;
                this.y = y;
                this.radius = radius;
                this.dx = dx;
                this.dy = dy;
                this.color = color;
            }

            draw() {
                context.beginPath();
                context.arc(this.x, this.y, this.radius, 0, Math.PI * 2);
                context.fillStyle = this.color;
                context.fill();
                context.closePath();
            }

            update() {
                this.draw();

                // Bounce off the walls
                if (this.x + this.dx > canvas.width - this.radius || this.x + this.dx < this.radius) {
                    this.dx = -this.dx;
                }
                if (this.y + this.dy > canvas.height - this.radius || this.y + this.dy < this.radius) {
                    this.dy = -this.dy;
                }

                this.x += this.dx;
                this.y += this.dy;
            }
        }

        // Create and initialize multiple balls
        for (let i = 0; i < 5; i++) {
            let x = Math.random() * (canvas.width - 2 * 20) + 20;
            let y = Math.random() * (canvas.height - 2 * 20) + 20;
            let radius = 20;
            let dx = (Math.random() - 0.5) * 4; // Random horizontal speed
            let dy = (Math.random() - 0.5) * 4; // Random vertical speed
            let color = `rgb(${Math.random() * 255},${Math.random() * 255},${Math.random() * 255})`;
            balls.push(new Ball(x, y, radius, dx, dy, color));
        }

        function animate() {
            requestAnimationFrame(animate);
            context.clearRect(0, 0, canvas.width, canvas.height);

            for (let i = 0; i < balls.length; i++) {
                balls[i].update();
            }
        }

        animate();
    </script>
</body>
</html>
""")
