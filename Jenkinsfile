pipeline {
    agent any

    stages {
        stage('Build') {
            steps {
                slackSend color: "#439FE0", message: "Build started: $BRANCH_NAME at $BUILD_URL"
                ansiColor('xterm') {
                    sh 'cmake -Bbuild -Hcorvis -DMKLROOT=False -DCMAKE_BUILD_TYPE=RelWithDebInfo -DRC_BUILD=`git rev-parse HEAD`'
                    sh 'cmake --build build -- -j'
                }
            }
        }
        stage('Test') {
            steps {
                sh 'cmake --build build --target test'
            }
        }
        stage('Build slam_client') {
            steps {
                sh '''#!/bin/bash
                    source ./corvis/src/movidius/mvenv
                    cd ./corvis/src/movidius/device
                    make -j
                '''
            }
        }
        stage('Run benchmark') {
            steps {
                withCredentials([string(credentialsId: 'slackBenchmarkToken', variable: 'SLACK_BENCHMARK_TOKEN')]) {
                    sh 'build/measure --benchmark /Data/new_test_suite/ --benchmark-output benchmark-$BRANCH_NAME-`git rev-parse HEAD`.txt --qvga'
                    sh 'cat benchmark-$BRANCH_NAME-`git rev-parse HEAD`.txt'
                    sh 'curl -F file=@benchmark-$BRANCH_NAME-`git rev-parse HEAD`.txt -F channels=#slam_build -F token=$SLACK_BENCHMARK_TOKEN https://slack.com/api/files.upload'
                }
            }
        }
    }
    post {
        always {
            deleteDir()
        }
        success {
            slackSend color: "good", message: "Build succeeded: $BRANCH_NAME at $BUILD_URL"
        }
        failure {
            slackSend color: "#FF0000", message: "Build failed: $BRANCH_NAME at $BUILD_URL"
        }
    }
}
