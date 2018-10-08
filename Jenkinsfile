pipeline {
    agent { label 'linux' }
    environment {
        CCACHE_BASEDIR = "${env.WORKSPACE}"
        CTEST_OUTPUT_ON_FAILURE = 1
    }
    stages {
        stage('Linux') {
            steps {
                slackSend color: "#439FE0", message: slack_build_message("started")
                sh "cmake -Bbuild -H. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DRC_BUILD=${env.GIT_COMMIT}"
                sh "cmake --build build -- -j"
                stash includes: 'build/measure', name: 'measure'
                stash includes: 'build/mv-usb-boot', name: 'mv-usb-boot'
                stash includes: 'build/hub', name: 'hub'
                dir("build/src") {
                    stash includes: 'libtracker.so', name: 'libtracker'
                }
            }
        }
        stage('Prepare Benchmark') {
            steps {
                sh 'cp -asf --no-preserve=mode $HOME/data/ $(realpath .)'
                sh 'rsync -a --chmod=ug+rw     $HOME/data/ $(realpath .)/data/ --include "*.json" --include "*/" --exclude "*"'
            }
        }
        stage('Test') {
            steps {
                sh 'cmake --build build --target test'
            }
            post {
                always {
                   junit testResults: 'build/**/*.xml', keepLongStdio: true
                }
            }
        }
        stage('Python') {
            steps {
                sh 'python2.7 -m py_compile scripts/*.py'
                sh 'python3   -m py_compile scripts/*.py'
            }
        }
        stage('Movidius') {
            steps {
                sh '''#!/bin/bash
                    source src/movidius/mvenv
                    export MV_TOOLS_DIR=$HOME/mdk/tools
                    export MV_COMMON_BASE="$(realpath --relative-to=src/movidius/device "$MV_COMMON_BASE")"
                    make -C src/movidius/device -j DirAppRelativeMdk=/src/movidius/device
                '''
                dir("src/movidius/device/output") {
                    stash name: 'device.mvcmd', includes: 'device.mvcmd'
                }
            }
        }
        stage('Check') {
            parallel {
                stage('Sync') {
                    steps {
                        sh 'build/measure --benchmark data/other/minimal_test_suite/'
                        sh 'build/measure --benchmark data/other/minimal_test_suite/ --disable-map'
                        sh 'build/measure --benchmark data/other/minimal_test_suite/ --no-fast-path'
                        sh 'build/measure             data/vr/WW50/VR_with_ctrl/Building/VR_RM_with_ctrl_yossi_3.stereo.rc --no-gui --relocalize --save-map test.map'
                        sh 'build/measure --benchmark data/other/minimal_test_suite/ --relocalize --load-map test.map'
                    }
                }
                stage('Async') {
                    steps {
                        sh 'build/measure --benchmark data/other/minimal_test_suite/ --async'
                    }
                }
            }
        }
        stage('Benchmark') {
            parallel {
                stage('CPU') {
                    steps {
                        sh 'build/measure --qvga --relocalize --benchmark data/ --pose-output %s.cpu.tum --benchmark-output benchmark-details-$BRANCH_NAME-$GIT_COMMIT.txt'
                        sh 'sed -ne \'/^Summary/,$p\' benchmark-details-$BRANCH_NAME-$GIT_COMMIT.txt > benchmark-summary-$BRANCH_NAME-$GIT_COMMIT.txt'
                        sh './run_safety_kpi.py data/vr .cpu.tum > kpi-safety-$BRANCH_NAME-$GIT_COMMIT.txt'
                        sh 'tail -n12 kpi-safety-$BRANCH_NAME-$GIT_COMMIT.txt >> benchmark-summary-$BRANCH_NAME-$GIT_COMMIT.txt'
                        sh 'find data/vr -type f -name "*.safetyvis.pdf" -print0 | tar -czf safetyvis-$GIT_COMMIT.tar.gz --null -T -'
                        sh 'echo "Visualization written to /tmp/safetyvis-$GIT_COMMIT.tar.gz" >> kpi-safety-$BRANCH_NAME-$GIT_COMMIT.txt'
                        sh 'rsync -a --chmod=ug+r safetyvis-$GIT_COMMIT.tar.gz /tmp/'
                        sh './run_kpis.py data/kpis .cpu.tum > kpi-details-$BRANCH_NAME-$GIT_COMMIT.txt'
                        sh 'tail -n14 kpi-details-$BRANCH_NAME-$GIT_COMMIT.txt >> benchmark-summary-$BRANCH_NAME-$GIT_COMMIT.txt'
                        archiveArtifacts artifacts: "kpi-*-$BRANCH_NAME-${GIT_COMMIT}.txt"
                        copyArtifacts projectName: "SlamTracker/master", filter: "benchmark-summary-master-*", target: "base"
                        sh 'diff -u base/benchmark-summary-master-* benchmark-summary-$BRANCH_NAME-* | sed  s@base/@@g | tee benchmark-changes-$BRANCH_NAME-$GIT_COMMIT.txt'
                        archiveArtifacts artifacts: "benchmark-*-$BRANCH_NAME-${GIT_COMMIT}.txt"
                        withCredentials([string(credentialsId: 'slackBenchmarkToken', variable: 'SLACK_BENCHMARK_TOKEN')]) {
                            sh 'curl -s -F file=@benchmark-changes-$BRANCH_NAME-$GIT_COMMIT.txt -F channels=#slam_build -F filetype=diff -F token=$SLACK_BENCHMARK_TOKEN https://slack.com/api/files.upload'
                       }
                    }
                }
                stage('Windows 32') {
                    agent { label 'windows' }
                    steps {
                        bat ([
                            '"%VS2017INSTALLDIR%/VC/Auxiliary/Build/vcvars32.bat"', 
                            'cmake -GNinja -Bbuild32 -H. -DCMAKE_BUILD_TYPE=Release -DRC_BUILD=${env.GIT_COMMIT}', 
                            'cmake --build build32 --config Release',
                            ].join(' && '))
                    }
                }
                stage('Windows 64') {
                    agent { label 'windows' }
                    steps {
                        bat ([
                            '"%VS2017INSTALLDIR%/VC/Auxiliary/Build/vcvars64.bat"',
                            'cmake -GNinja -Bbuild64 -H. -DCMAKE_BUILD_TYPE=Release -DRC_BUILD=${env.GIT_COMMIT}', 
                            'cmake --build build64 --config Release',
                            ].join(' && '))
                    }
                }
                stage('TM2') {
                    agent { label 'tm2' }
                    options { skipDefaultCheckout() }
                    steps {
                        unstash 'hub'
                        sh 'build/hub --disableport 0,1 --enableport 0,1 --delay 100'
                        unstash 'mv-usb-boot'
                        unstash 'device.mvcmd'
                        sh 'build/mv-usb-boot device.mvcmd'
                        unstash 'measure'
                        unstash 'libtracker'
                        sh 'build/mv-usb-boot -v 0x040E -p 0xF63B :re'
                        timeout(time: 10, unit: 'MINUTES') {
                            sh 'build/measure --tm2 --no-gui --relocalize "$HOME/data/vr/WW50/VR_with_ctrl/Shooter(Raw_Data)/VR_RD_with_ctrl_1.stereo.rc"'
                        }
                    }
                }
            }
        }
    }
    post {
        always {
            deleteDir()
        }
        success {
            slackSend color: "good", message: slack_build_message("succeeded (<${env.BUILD_URL}/artifact/benchmark-details-$BRANCH_NAME-${env.GIT_COMMIT}.txt|benchmark>"
                                                                  +         " <${env.BUILD_URL}/artifact/benchmark-summary-$BRANCH_NAME-${env.GIT_COMMIT}.txt|summary>)")
        }
        failure {
            slackSend color: "#FF0000", message: slack_build_message("<$BUILD_URL/console|failed>")
        }
        aborted {
            slackSend color: "#FF00FF", message: slack_build_message("<$BUILD_URL/console|aborted>")
        }
    }
}

def slack_build_message(status) {
    def q = { s -> s.replaceAll('&','&amp;').replaceAll('<','&lt;').replaceAll('>','&gt;') }
    "<$JOB_DISPLAY_URL|Build> <$RUN_DISPLAY_URL|#$BUILD_ID> of " + (env.CHANGE_ID
        ? "<${env.CHANGE_URL}|#${q env.CHANGE_ID} ${q env.CHANGE_TITLE}> for `${q env.CHANGE_TARGET}` by `${env.CHANGE_AUTHOR}`"
        : "<${env.GIT_URL.replaceAll(/\.git$/,'')}/tree/${env.GIT_BRANCH}|${q env.BRANCH_NAME}>") +
    " with <$RUN_CHANGES_DISPLAY_URL|changes> $status"
}
