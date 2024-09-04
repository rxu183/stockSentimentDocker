# Step 1: Use an official Node.js runtime as a parent image to build the React app
FROM node:18-alpine as build

# Step 2: Set the working directory inside the container
WORKDIR /app

# Step 3: Copy the package.json and package-lock.json to the working directory
COPY package*.json ./

# Step 4: Install the dependencies
RUN npm install

# Step 5: Copy the rest of the application code to the working directory
COPY . .

# Step 6: Build the React app for production
RUN npm run build

# Step 7: Use a lightweight Nginx image to serve the built app
FROM nginx:stable-alpine

# Step 8: Copy the build files from the previous stage to the Nginx web root
COPY --from=build /app/build /usr/share/nginx/html

# Step 9: Expose the port that Nginx will use
EXPOSE 80

# Step 10: Start Nginx when the container starts
CMD ["nginx", "-g", "daemon off;"]