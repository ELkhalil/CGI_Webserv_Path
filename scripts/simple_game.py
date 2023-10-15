#!/usr/bin/env python

# Set the content type to HTML
print("Content-Type: text/html\n")

# HTML code to create a canvas and animate a bouncing ball
print("""
<!DOCTYPE html>
<html>
<head>
    <title>CGI Bouncing Ball</title>
</head>
<body>
    <h1>Python CGI Bouncing Ball</h1>
    <canvas id="myCanvas" width="400" height="200" style="border:1px solid #000;"></canvas>
    <script>
        var canvas = document.getElementById("myCanvas");
        var context = canvas.getContext("2d");
        var ballX = canvas.width / 2;
        var ballY = canvas.height - 30;
        var ballRadius = 10;
        var dx = 2;
        var dy = -2;

        function drawBall() {
            context.clearRect(0, 0, canvas.width, canvas.height);
            context.beginPath();
            context.arc(ballX, ballY, ballRadius, 0, Math.PI * 2);
            context.fillStyle = "blue";
            context.fill();
            context.closePath();

            // Bounce off the walls
            if (ballX + dx > canvas.width - ballRadius || ballX + dx < ballRadius) {
                dx = -dx;
            }
            if (ballY + dy > canvas.height - ballRadius || ballY + dy < ballRadius) {
                dy = -dy;
            }

            ballX += dx;
            ballY += dy;
        }

        setInterval(drawBall, 10); // Call drawBall every 10 milliseconds for animation
    </script>
</body>
</html>
""")
