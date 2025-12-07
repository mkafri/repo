pipeline {
    // Run on any agent that has Docker and SSH client installed
    agent any

    environment {
        REGISTRY   = 'registry.example.com'
        IMAGE_NAME = 'my-node-app'
        IMAGE_TAG  = "${env.BUILD_NUMBER}"
    }

    stages {
        stage('Checkout') {
            steps {
                echo 'Checking out source code from Git...'
                checkout scm
            }
        }

        stage('Install & Test') {
            steps {
                echo 'Installing dependencies and running tests...'
                sh '''
                    npm ci
                    npm test
                '''
            }
        }

        stage('Build Docker Image') {
            steps {
                echo "Building Docker image ${REGISTRY}/${IMAGE_NAME}:${IMAGE_TAG}..."
                sh """
                    docker build -t ${REGISTRY}/${IMAGE_NAME}:${IMAGE_TAG} .
                """
            }
        }

        stage('Push Image') {
            steps {
                echo 'Pushing Docker image to registry...'
                sh """
                    docker login ${REGISTRY} -u $REG_USER -p $REG_PASS
                    docker push ${REGISTRY}/${IMAGE_NAME}:${IMAGE_TAG}
                """
            }
        }

        stage('Deploy to Staging') {
            steps {
                echo 'Deploying container to staging server...'
                sh """
                    ssh deploy@staging-server '
                        docker pull ${REGISTRY}/${IMAGE_NAME}:${IMAGE_TAG} &&
                        docker stop my-node-app || true &&
                        docker rm my-node-app || true &&
                        docker run -d --name my-node-app -p 80:3000 ${REGISTRY}/${IMAGE_NAME}:${IMAGE_TAG}
                    '
                """
            }
        }
    }

    post {
        success {
            echo 'Deployment successful!'
        }
        failure {
            echo 'Deployment failed. Please check Jenkins logs.'
        }
    }
}
