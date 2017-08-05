#!groovy

pipeline {
  agent any

  options {
    buildDiscarder(logRotator(numToKeepStr: '30'))
    timeout(time: 1, unit: 'HOURS')
    timestamps()
  }

  stages {
    stage('lib') {
      steps {
        dir('lib') {
          sh 'make clean'
          sh 'make test'
        }
      }
    }

    stage('simulator') {
      steps {
        dir('simulator') {
          sh 'make clean'
          sh 'make'
          sh 'make test'
        }
      }
    }

    stage('ai') {
      steps {
        parallel(
          pass: {
            dir('pass') {
              sh 'make clean'
              sh 'make'
            }
          },
          random: {
            dir('random') {
              sh 'make clean'
              sh 'make'
            }
          },
          greedy: {
            dir('greedy') {
              sh 'make clean'
              sh 'make'
            }
          },
          failFast: false)
      }
    }
  }

  post {
    failure {
      script {
        if (env.BRANCH_NAME == 'master') {
          slackSend(color: 'danger', message: "BUILD FAILED <${env.BUILD_URL}console|Jenkins Console>")
        }
      }
    }
  }
}
