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
          sh 'make test'
        }
      }
    }

    stage('simulator') {
      steps {
        dir('simulator') {
          sh 'make'
          sh 'make test'
        }
      }
    }
  }
}
