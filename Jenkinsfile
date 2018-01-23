pipeline {
    agent { label 'linux' }
    environment {
        CCACHE_BASEDIR = "${env.WORKSPACE}"
    }
    stages {
        stage('Build') {
            parallel {
                stage('Linux') {
                    steps {
                        slackSend color: "#439FE0", message: slack_build_message("started")
                        sh "cmake -Bbuild -Hcorvis -DMKLROOT=False -DCMAKE_BUILD_TYPE=RelWithDebInfo -DRC_BUILD=${env.GIT_COMMIT}"
                        sh "cmake --build build -- -j"
                    }
                }
                stage('Windows 32') {
                    agent { label 'windows' }
                    steps {
                        bat "cmake -Bbuild-x32 -Hcorvis -DMKLROOT=False -DCMAKE_BUILD_TYPE=RelWithDebInfo -DRC_BUILD=${env.GIT_COMMIT} -A Win32"
                        bat "cmake --build build-x32 --config RelWithDebInfo"
                    }
                }
                stage('Windows 64') {
                    agent { label 'windows' }
                    steps {
                        bat "cmake -Bbuild-x64 -Hcorvis -DMKLROOT=False -DCMAKE_BUILD_TYPE=RelWithDebInfo -DRC_BUILD=${env.GIT_COMMIT} -A x64"
                        bat "cmake --build build-x64 --config RelWithDebInfo"
                    }
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
                    source  corvis/src/movidius/mvenv
                    make -C corvis/src/movidius/device -j
                '''
            }
        }
        stage('Run benchmark') {
            steps {
                withCredentials([string(credentialsId: 'slackBenchmarkToken', variable: 'SLACK_BENCHMARK_TOKEN')]) {
                    sh 'build/measure --qvga --benchmark $JENKINS_HOME/benchmark_data/new_test_suite/ --benchmark-output benchmark-details-$BRANCH_NAME-$GIT_COMMIT.txt'
                    sh 'sed -ne /^Length/,//p benchmark-details-$BRANCH_NAME-$GIT_COMMIT.txt                           > benchmark-summary-$BRANCH_NAME-$GIT_COMMIT.txt'
                    sh 'curl -s -F file=@benchmark-summary-$BRANCH_NAME-$GIT_COMMIT.txt -F channels=#slam_build -F token=$SLACK_BENCHMARK_TOKEN https://slack.com/api/files.upload'
                    archiveArtifacts artifacts: "benchmark-*-$BRANCH_NAME-${GIT_COMMIT}.txt"
                }
            }
        }
    }
    post {
        always {
            deleteDir()
        }
        success {
            slackSend color: "good", message: slack_build_message("succeeded (<${env.BUILD_URL}/artifact/benchmark-details-$BRANCH_NAME-${env.GIT_COMMIT}.txt|benchmark>)")
        }
        failure {
            slackSend color: "#FF0000", message: slack_build_message("<$BUILD_URL/console|failed>")
        }
    }
}

def slack_build_message(status) {
    def q = { s -> s.replaceAll('&','&amp;').replaceAll('<','&lt;').replaceAll('>','&gt;') }
    "<$JOB_URL|Build> <$BUILD_URL|#$BUILD_ID> of " + (env.CHANGE_ID
        ? "<${env.CHANGE_URL}|#${q env.CHANGE_ID} ${q env.CHANGE_TITLE}> for `${q env.CHANGE_TARGET}` by `${env.CHANGE_AUTHOR}`"
        : "<${env.GIT_URL.replaceAll(/\.git$/,'')}/tree/${env.GIT_BRANCH}|${q env.BRANCH_NAME}>") +
    " $status"
}
